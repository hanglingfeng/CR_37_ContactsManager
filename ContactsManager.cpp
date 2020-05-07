//在指定文件内存储联系人
//1.增加: 对联系人的增加操作，每次输入不超过100字节
//2.删除：对联系人的删除操作
//3.修改：修改指定的联系人，如果空间长度不够则存储在其他地方，原联系人视为被删除
//4.查询：按联系人各个字段查找联系人的基本信息，支持模糊查找
//5.统计：统计每个汉字和字母的出现次数和比例。
//6.显示存储信息：按顺序显示已分配(U)、未分配(F)资源,例如10字节空间，显示UFUUFFFUUU
//7.碎片整理：增删改中出现了不连续的“孔隙”，整理后使得这些“孔隙”连续且可用

//open close read save

#include "ContactsManager.h"

#define BIAS 0x10

static char szFileName[] = "data.bin";//数据文件，前16字节保存savingIndex, validIndex, deprecatedIndex
static FILE *fpData;

#define TABLE_SIZE 100
static int g_itemIndexTable[TABLE_SIZE] = { -1 };//保存所有item在文件中的下标，遍历时发现-1，则停止遍历
static int g_nSavingIndex = 0;//当前可用位置的下标，新增item时保存在此处
//int g_nValidIndex = -1;//有效内容的下标
//int g_nDeprecatedIndex = -1;//弃用内容的下标


bool OpenDevice() {
    fpData = fopen(szFileName, "rb+");
    if (fpData) {
        return true;
    }
    return false;
}

void CloseDevice() {
    if (fpData) {
        if (fclose(fpData) == EOF) {
            exit(EXIT_FAILURE);
        }
        fpData = NULL;
    }
}

//初始化函数，第一个调用，设置几个全局变量的值。如果不是第一次使用文件，则读取g_nSavingIndex
void InitializeIndexData() {
    bool bSuccess = true;
    if (OpenDevice()) {
        if (fread(&g_nSavingIndex, sizeof(int), 1, fpData) == 1
            //&& fread(&g_nValidIndex, sizeof(int), 1, fpData) == 1 
            //&& fread(&g_nDeprecatedIndex, sizeof(int), 1, fpData) == 1
            ) {
            if (g_nSavingIndex == 0) {//一个item都没有，例如第一次使用或删光数据
                g_nSavingIndex = BIAS;//前BIAS个字节始终占用，不存item数据
                //g_nValidIndex = -1;
                //g_nDeprecatedIndex = -1;
            }
        }
        else {
            bSuccess = false;
        }
    }
    else {
        bSuccess = false;
    }
    CloseDevice();
    if (!bSuccess) {
        exit(EXIT_FAILURE);
    }
}



//寻找下一个item存放的下标
//参数：
//	无
//返回值：
//	int：返回找到的下标，没找到返回-1 
int GetNextSavingIndex() {
    if (OpenDevice()) {
        int nNextSavingIndex = 0;
        int nTempIndex = 0;
        while (true) {
            unsigned char chItemSize = 0;
            fread(&chItemSize, sizeof(chItemSize), 1, fpData);
            if (ferror(fpData)) {//ferror则返回-1
                return -1;
            }
            else if (feof(fpData)) {//跳出循环的原因是feof
                break;
            }
            if (chItemSize > 0) {//大于0说明保存了大小数据                
                int nFilePos = ftell(fpData);
                if (nFilePos == -1) {
                    return -1;
                }
                else {
                    nNextSavingIndex = nFilePos - 1;//保存itemSize所在下标
                }
                if (fseek(fpData, nNextSavingIndex + chItemSize, SEEK_SET) != 0) {
                    return -1;
                }
            }
        }
        return nNextSavingIndex;
    }
    return -1;
}

//在下标为nIndex的弃用空间写入item,返回是否成功写入
bool WriteItemInDeprecatedIndex(int nIndex, const Item *pItem) {
    int nNextValidIndex = GetFirstValidIndex(nIndex);//要写入的数据在此下标    
    int nFreeSize = nNextValidIndex - nIndex;//多少空间可供写入
    if (nNextValidIndex == -1) {//-1说明nIndex之后不存在有效数据，可以直接在nIndex处写入
        g_nSavingIndex = nIndex;//由于后面没有有效数据，直接赋值
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fwrite(&g_nSavingIndex, sizeof(g_nSavingIndex), 1, fpData) != 1) {//更新文件中的g_nSavingIndex
            exit(EXIT_FAILURE);
        }
        CloseDevice();
        return false;//交给上一层函数专门处理在g_nSavingIndex处写入的情况
    }

    bool bSuccess = true;
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }
    if (fseek(fpData, nIndex, SEEK_SET) != 0) {//定位
        exit(EXIT_FAILURE);
    }
    int nNewItemSize = pItem->head.chItemSize;
    if (nNewItemSize <= nFreeSize) {
        if (fwrite(pItem, pItem->head.chItemSize, 1, fpData) != 1) {//写入item
            exit(EXIT_FAILURE);
        }

        unsigned char chZero = 0;
        unsigned char chLeftSize = nFreeSize - nNewItemSize;//剩余多少空间没用

        if (chLeftSize >= 1) {//只剩1字节空余空间或更多时
            if (fwrite(&chLeftSize, sizeof(chLeftSize), 1, fpData) != 1) {//size写入进去
                exit(EXIT_FAILURE);
            }
        }
        if (chLeftSize >= 2) {//只剩2字节空余空间或更多时
            if (fwrite(&chZero, sizeof(chZero), 1, fpData) != 1) {//写入0,算deprecated空间
                exit(EXIT_FAILURE);
            }
        }
    }
    else {
        bSuccess = false;//空间不足，上一层函数会继续写入
    }
    CloseDevice();
    return bSuccess;    
}

//返回成功添加的item所在文件中的下标
int AddItem(const Item *pItem) {
    int nWriteIndex = GetFirstDeprecatedIndex(BIAS);//要写入的数据在此下标
    if (nWriteIndex >= BIAS) {//找到了弃用空间
        if (WriteItemInDeprecatedIndex(nWriteIndex, pItem)) {
            return nWriteIndex;//成功写入则返回，否则执行后续写入方案
        }        
    }    

    int nIndex = -1;
    if (!OpenDevice()) {
        goto ErrorProc;
    }    

    if (fseek(fpData, g_nSavingIndex, SEEK_SET) != 0) {//写item前，先设置文件指针到空白区域
        goto ErrorProc;
    }
    if (fwrite(pItem, pItem->head.chItemSize, 1, fpData) != 1) {//写入item
        goto ErrorProc;
    }
    g_nSavingIndex += pItem->head.chItemSize;//更新g_nSavingIndex，便于下一次写入

    if (fseek(fpData, 0, SEEK_SET) != 0) {//写g_nSavingIndex前，先设置文件指针到开头
        goto ErrorProc;
    }
    if (fwrite(&g_nSavingIndex, sizeof(g_nSavingIndex), 1, fpData) != 1) {//写g_nSavingIndex
        goto ErrorProc;
    }
    nIndex = g_nSavingIndex - pItem->head.chItemSize;//写入的item所在文件的下标
ErrorProc:
    CloseDevice();
    return nIndex;
}


//清空g_itemTable内容
//参数：
//	无
//返回值：
//	无
void EmptyItemTable() {
    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        g_itemIndexTable[i] = -1;
    }
}


//寻找第一个已使用位置的下标,参数代表从哪里开始搜索,如果空间尚未存放任何有效字符串，返回-1
//参数：
//	int nStartIndex [in]从哪个下标开始搜索
//返回值：
//	int：返回找到的下标，没找到返回-1 
int GetFirstValidIndex(int nStartIndex) {
    bool bSuccess = true;
    Item item = { 0 };
    tagHead head = { 0 };

    if (!OpenDevice()) {
        bSuccess = false;
        goto ErrorProc;
    }
    if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//先跳过前nStartIndex字节
        bSuccess = false;
        goto ErrorProc;
    }
    while (nStartIndex < g_nSavingIndex) {
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            bSuccess = false;
            goto ErrorProc;
        }
        if (head.chItemSize > sizeof(Item) && head.bIsUsed) {//大于sizeof(Item)且在使用，才算有效
            break;//找到了，跳出循环
        }
        else {
            nStartIndex += head.chItemSize;//加上头部Item的字节数
            if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//设置新位置
                bSuccess = false;
                goto ErrorProc;
            }
        }
    }
ErrorProc:
    CloseDevice();
    if (!bSuccess) {
        exit(EXIT_FAILURE);
    }
    if (nStartIndex == g_nSavingIndex) {//相等说明没有更多了
        nStartIndex = -1;
    }
    return nStartIndex;
}

//寻找第一个已弃用位置的下标,参数代表从哪里开始搜索,如果空间尚未存放任何有效字符串，返回-1
//参数：
//	int nStartIndex [in]从哪个下标开始搜索
//返回值：
//	int：返回找到的下标，没找到返回-1 
int GetFirstDeprecatedIndex(int nStartIndex) {
    bool bSuccess = true;
    Item item = { 0 };
    tagHead head = { 0 };

    if (!OpenDevice()) {
        bSuccess = false;
        goto ErrorProc;
    }
    if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//先跳过前nStartIndex字节
        bSuccess = false;
        goto ErrorProc;
    }
    while (nStartIndex < g_nSavingIndex) {
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            bSuccess = false;
            goto ErrorProc;
        }
        if (!(head.chItemSize > sizeof(Item) && head.bIsUsed)) {//有效的条件取反
            break;//找到了，跳出循环
        }
        else {
            nStartIndex += head.chItemSize;//加上头部Item的字节数
            if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//设置新位置
                bSuccess = false;
                goto ErrorProc;
            }
        }
    }
ErrorProc:
    CloseDevice();
    if (!bSuccess) {
        exit(EXIT_FAILURE);
    }
    if (nStartIndex == g_nSavingIndex) {//相等说明没有更多了
        nStartIndex = -1;
    }
    return nStartIndex;
}

//显示所有item信息
void ShowAllItem() {
    GetAllItem();
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }
    Item *pItem = (Item *)malloc(sizeof(Item) + INPUT_LENGTH);
    if (!pItem) {
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        if (g_itemIndexTable[i] != -1) {
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//定位，准备读取validIndex对应的item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//读取一个item
                exit(EXIT_FAILURE);
            }
            printf("下标：%x, 姓名：%s, 电话：%llu\n", g_itemIndexTable[i], pItem->szName, pItem->ullPhone);            
        }
    }
    CloseDevice();
    free(pItem);
    pItem = NULL;    
}

//获取所有item所在下标，存在g_itemTable里
//参数：
//	无
//返回值：
//	无
void GetAllItem() {
    EmptyItemTable();
    int nValidIndex = GetFirstValidIndex(BIAS);
    int i = 0;

    while (nValidIndex >= BIAS && nValidIndex < g_nSavingIndex) {
        //printf("%x\n", nValidIndex);
        g_itemIndexTable[i] = nValidIndex;
        ++i;

        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fseek(fpData, nValidIndex, SEEK_SET) != 0) {//定位到nValidIndex
            exit(EXIT_FAILURE);
        }
        tagHead head = { 0 };
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            exit(EXIT_FAILURE);
        }
        CloseDevice();
        nValidIndex = GetFirstValidIndex(nValidIndex + head.chItemSize);//定位下一个可能的下标
    }
}

//在itemTable中检查传入的item是否存在
//参数：
//	int index [in]要搜索item在文件中的下标
//返回值：
//	bool	存在返回true，否则返回false
bool SearchItem(int nIndex) {
    bool bFound = false;
    GetAllItem();
    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        if (g_itemIndexTable[i] == -1) {//-1代表结束
            break;
        }
        if (g_itemIndexTable[i] == nIndex) {
            bFound = true;
            break;
        }
    }
    return bFound;
}

//移除item，返回true代表成功,false代表失败
//参数：
//	Item *pItem [in]要移除item在文件中的下标
//返回值：
//	bool	返回true代表成功,false代表失败
bool RemoveItem(int nIndex) {
    bool bFound = SearchItem(nIndex);
    if (bFound) {
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fseek(fpData, nIndex + 1, SEEK_SET) != 0) {//+1定位到isUsed字段
            exit(EXIT_FAILURE);
        }
        bool isUsed = false;
        if (fwrite(&isUsed, sizeof(isUsed), 1, fpData) != 1) {////写入item
            exit(EXIT_FAILURE);
        }
    }
    CloseDevice();
    return bFound;
}

//修改item
//参数：
//	int index [in]要修改哪个下标对应的item
//	const Item *pItem [in]新的item
//返回值：
//	bool 返回true代表成功,false代表失败
bool ModifyItem(int nIndex, const Item *pItem) {
    if (SearchItem(nIndex)) {
        int nNextValidIndex = 0;
        for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
            if (g_itemIndexTable[i] == nIndex) {
                nNextValidIndex = g_itemIndexTable[i + 1];
                break;
            }
        }
        int nFreeSize = nNextValidIndex - nIndex;//多少空间可供写入
        if (nNextValidIndex == -1) {//-1说明参数index是最后一个validIndex，不存在空隙
            nFreeSize = pItem->head.chItemSize;
        }

        bool bSuccess = true;
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fseek(fpData, nIndex, SEEK_SET) != 0) {//定位
            exit(EXIT_FAILURE);
        }
        int nNewItemSize = pItem->head.chItemSize;
        if (nNewItemSize <= nFreeSize) {
            if (fwrite(pItem, pItem->head.chItemSize, 1, fpData) != 1) {//写入item
                exit(EXIT_FAILURE);
            }

            unsigned char chZero = 0;
            unsigned char chLeftSize = nFreeSize - nNewItemSize;//剩余多少空间没用

            if (chLeftSize >= 1) {//只剩1字节空余空间或更多时
                if (fwrite(&chLeftSize, sizeof(chLeftSize), 1, fpData) != 1) {//size写入进去
                    exit(EXIT_FAILURE);
                }
            }
            if (chLeftSize >= 2) {//只剩2字节空余空间或更多时
                if (fwrite(&chZero, sizeof(chZero), 1, fpData) != 1) {//写入0,算deprecated空间
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            RemoveItem(nIndex);//空间放不下则直接删除并添加
            AddItem(pItem);
        }
        CloseDevice();
        return bSuccess;
    }
    else {
        return false;
    }
}

//碎片整理，使文件左侧都是已使用空间，右侧都是未使用空间
//参数：
//	无
//返回值：
//	无
void Defragment() {
    int nDeprecatedIndex = GetFirstDeprecatedIndex(BIAS);
    if (nDeprecatedIndex == -1) {//无碎片，直接返回
        return;
    }

    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }
    if (fseek(fpData, nDeprecatedIndex, SEEK_SET) != 0) {//定位
        exit(EXIT_FAILURE);
    }
    tagHead head = { 0 };
    if (fread(&head, sizeof(head), 1, fpData) != 1) {
        exit(EXIT_FAILURE);
    }

    int nValidIndex = GetFirstValidIndex(nDeprecatedIndex + head.chItemSize);
    Item *pItem = NULL;
    while (nDeprecatedIndex >= 0 && nValidIndex > nDeprecatedIndex) {//继续搜索的条件
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fseek(fpData, nValidIndex, SEEK_SET) != 0) {//定位
            exit(EXIT_FAILURE);
        }
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            exit(EXIT_FAILURE);
        }
        Item *pTemp = (Item *)realloc(pItem, head.chItemSize);//引入pTemp，防止返回NULL覆盖pItem，而后者还来不及free
        if (!pTemp) {
            exit(EXIT_FAILURE);
        }
        pItem = pTemp;
        if (fseek(fpData, nValidIndex, SEEK_SET) != 0) {//定位
            exit(EXIT_FAILURE);
        }
        if (fread(pItem, head.chItemSize, 1, fpData) != 1) {
            exit(EXIT_FAILURE);
        }

        if (fseek(fpData, nDeprecatedIndex, SEEK_SET) != 0) {//定位
            exit(EXIT_FAILURE);
        }
        if (fwrite(pItem, head.chItemSize, 1, fpData) != 1) {///写入item
            exit(EXIT_FAILURE);
        }
        if (fflush(fpData) == EOF) {
            exit(EXIT_FAILURE);
        }

        nDeprecatedIndex += head.chItemSize;//前移Item的总字节数，下一次转移数据到此位置
        nValidIndex += head.chItemSize;//前移Item的总字节数，下一次从此位置开始搜索
        nValidIndex = GetFirstValidIndex(nValidIndex);//定位下一个有效下标
    }
    free(pItem);
    pItem = NULL;
    g_nSavingIndex = nDeprecatedIndex;//下一个新字符串的保存在此下标
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }
    if (fseek(fpData, 0, SEEK_SET) != 0) {//定位开头，写入g_nSavingIndex
        exit(EXIT_FAILURE);
    }
    if (fwrite(&g_nSavingIndex, sizeof(g_nSavingIndex), 1, fpData) != 1) {//写入g_nSavingIndex
        exit(EXIT_FAILURE);
    }
    CloseDevice();
}



//显示存储信息
//参数：
//	无
//返回值：
//	无
void ShowInformation() {
    puts("UUUUUUUU UUUUUUUU");//前0x10字节总是占用
    int nValidIndex = GetFirstValidIndex(BIAS);

    GetAllItem();
    int itemCount = 0;
    for (; itemCount < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++itemCount) {//计算总个数
        if (g_itemIndexTable[itemCount] == -1) {
            break;
        }
    }

    //准备保存每个validIndex对应的size，准备在申请的空间中存放每个item占用字节数
    unsigned char *pSizeTable = (unsigned char *)malloc(sizeof(char) * itemCount);
    if (!pSizeTable) {
        exit(EXIT_FAILURE);
    }
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < itemCount; i++) {
        if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//定位，准备读取validIndex对应的size
            exit(EXIT_FAILURE);
        }
        if (fread(pSizeTable + i, sizeof(char), 1, fpData) != 1) {//读取validIndex对应的size
            exit(EXIT_FAILURE);
        }
    }

    int nPrintCount = 0;
    for (int i = BIAS; i < g_nSavingIndex; i++) {//g_nSavingIndex开始都是未使用空间
        bool bInRange = false;
        for (int j = 0; j < itemCount; j++) {
            if (i >= g_itemIndexTable[j] && i < g_itemIndexTable[j] + pSizeTable[j]) {
                //i代表在文件中的下标，处于范围内则打印U
                bInRange = true;
                break;
            }
        }
        if (bInRange) {
            putchar('U');
        }
        else {
            putchar('F');
        }
        ++nPrintCount;
        if (nPrintCount % 16 == 0) {
            putchar('\n');
        }
        else if (nPrintCount % 8 == 0) {
            putchar(' ');
        }
    }
    puts("F...");
    free(pSizeTable);
    pSizeTable = NULL;
}

//显示每个字母的出现次数和比例
//参数：
//	无
//返回值：
//	无
void ShowEachCharInformation() {
    int chAlphabet[26 + 6 + 26] = { 0 };//ASCII中Z和a之间有6个字符，将来遍历时直接忽略这6个下标
    int nCharCount = 0;//字母个数
    int nWordCount = 0;//汉字个数    
    tagWordInfo words[INPUT_LENGTH * 10] = { 0 };
    int index = 0;//words的下标
    GetAllItem();
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }

    Item *pItem = (Item *)malloc(sizeof(Item) + INPUT_LENGTH);
    if (!pItem) {
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        if (g_itemIndexTable[i] != -1) {
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//定位，准备读取validIndex对应的item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//读取一个item
                exit(EXIT_FAILURE);
            }

            int nStringSize = pItem->head.chItemSize - sizeof(Item);
            for (int j = 0; j < nStringSize - 1; ++j) {//-1去除'\0'

                char ch = pItem->szName[j];//获得一个字符串的每个字符
                if ((unsigned)ch < 128) {
                    if (isalpha(ch)) {
                        ++nCharCount;
                        ++chAlphabet[ch - 'A'];//大写A在下标0，B在下标1，Z在下标25。小写a在下标32
                    }
                }
                else {
                    ++nWordCount;
                    tagWordInfo info = { 1,(unsigned)pItem->szName[j],(unsigned)pItem->szName[j + 1] };//大于等于128的一次读2个
                    bool bExist = false;
                    for (size_t k = 0; k < sizeof(words) / sizeof(words[0]); k++) {
                        if (words[k].bLowByte == info.bLowByte && words[k].bHighByte == info.bHighByte) {
                            ++words[k].nCount;//汉字已存在则直接递增个数
                            bExist = true;
                            break;
                        }
                    }
                    if (!bExist) {
                        words[index] = info;//汉字不存在则直接赋值
                    }
                    ++index;
                    ++j;//由于一次读取了两个，所以递增
                }
            }
        }
    }
    free(pItem);
    pItem = NULL;
    if (nCharCount) {
        for (int i = 0; i < 26; ++i) {
            if (chAlphabet[i]) {
                printf("%2c 出现%5d次，占比%6.2f%%\n", 'A' + i, chAlphabet[i], 100 * (double)chAlphabet[i] / nCharCount);
            }            
        }
        for (int i = 0; i < 26; ++i) {
            if (chAlphabet[i + 26 + 6]) {
                printf("%2c 出现%5d次，占比%6.2f%%\n", 'a' + i, chAlphabet[i + 26 + 6], 100 * (double)chAlphabet[i + 26 + 6] / nCharCount);
            }            
        }
    }
    if (nWordCount) {
        for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
            if (words[i].bLowByte) {
                printf("%c%c 出现%5d次，占比%6.2f%%\n", (char)words[i].bLowByte, (char)words[i].bHighByte,
                    words[i].nCount, 100 * (double)words[i].nCount / nWordCount);

            }
        }

    }
}

//查寻字符串,支持模糊查找
//参数：
//	const char *pSubString [in]要匹配的字符串
//返回值：
//	void
void QueryItemByName(const char *pSubString) {
    bool bFound = false;
    GetAllItem();
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }

    Item *pItem = (Item *)malloc(sizeof(Item) + INPUT_LENGTH);
    if (!pItem) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        if (g_itemIndexTable[i] != -1) {
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//定位，准备读取validIndex对应的item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//读取一个item
                exit(EXIT_FAILURE);
            }

            if (strstr((char *)pItem->szName, pSubString)) {
                bFound = true;
                printf("下标：%x, 姓名：%s, 电话：%llu\n", g_itemIndexTable[i], pItem->szName, pItem->ullPhone);
            }
        }
    }
    free(pItem);
    pItem = NULL;
    if (!bFound) {
        printf("未找到匹配项\n");
    }
}

//查寻字符串,支持模糊查找
//参数：
//	const char *pSubString [in]要匹配的字符串
//返回值：
//	void
void QueryItemByPhone(const char *pSubString) {
    bool bFound = false;
    GetAllItem();
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }

    Item *pItem = (Item *)malloc(sizeof(Item) + INPUT_LENGTH);
    if (!pItem) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        if (g_itemIndexTable[i] != -1) {
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//定位，准备读取validIndex对应的item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//读取一个item
                exit(EXIT_FAILURE);
            }
            
            char szBuff[21] = { 0 };
            _ui64toa(pItem->ullPhone, szBuff, 10);
            if (strstr(szBuff, pSubString)) {
                bFound = true;
                printf("下标：%x, 姓名：%s, 电话：%llu\n", g_itemIndexTable[i], pItem->szName, pItem->ullPhone);
            }
        }
    }
    free(pItem);
    pItem = NULL;
    if (!bFound) {
        printf("未找到匹配项\n");
    }
}