//��ָ���ļ��ڴ洢��ϵ��
//1.����: ����ϵ�˵����Ӳ�����ÿ�����벻����100�ֽ�
//2.ɾ��������ϵ�˵�ɾ������
//3.�޸ģ��޸�ָ������ϵ�ˣ�����ռ䳤�Ȳ�����洢�������ط���ԭ��ϵ����Ϊ��ɾ��
//4.��ѯ������ϵ�˸����ֶβ�����ϵ�˵Ļ�����Ϣ��֧��ģ������
//5.ͳ�ƣ�ͳ��ÿ�����ֺ���ĸ�ĳ��ִ����ͱ�����
//6.��ʾ�洢��Ϣ����˳����ʾ�ѷ���(U)��δ����(F)��Դ,����10�ֽڿռ䣬��ʾUFUUFFFUUU
//7.��Ƭ������ɾ���г����˲������ġ���϶���������ʹ����Щ����϶�������ҿ���

//open close read save

#include "ContactsManager.h"

#define BIAS 0x10

static char szFileName[] = "data.bin";//�����ļ���ǰ16�ֽڱ���savingIndex, validIndex, deprecatedIndex
static FILE *fpData;

#define TABLE_SIZE 100
static int g_itemIndexTable[TABLE_SIZE] = { -1 };//��������item���ļ��е��±꣬����ʱ����-1����ֹͣ����
static int g_nSavingIndex = 0;//��ǰ����λ�õ��±꣬����itemʱ�����ڴ˴�
//int g_nValidIndex = -1;//��Ч���ݵ��±�
//int g_nDeprecatedIndex = -1;//�������ݵ��±�


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

//��ʼ����������һ�����ã����ü���ȫ�ֱ�����ֵ��������ǵ�һ��ʹ���ļ������ȡg_nSavingIndex
void InitializeIndexData() {
    bool bSuccess = true;
    if (OpenDevice()) {
        if (fread(&g_nSavingIndex, sizeof(int), 1, fpData) == 1
            //&& fread(&g_nValidIndex, sizeof(int), 1, fpData) == 1 
            //&& fread(&g_nDeprecatedIndex, sizeof(int), 1, fpData) == 1
            ) {
            if (g_nSavingIndex == 0) {//һ��item��û�У������һ��ʹ�û�ɾ������
                g_nSavingIndex = BIAS;//ǰBIAS���ֽ�ʼ��ռ�ã�����item����
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



//Ѱ����һ��item��ŵ��±�
//������
//	��
//����ֵ��
//	int�������ҵ����±꣬û�ҵ�����-1 
int GetNextSavingIndex() {
    if (OpenDevice()) {
        int nNextSavingIndex = 0;
        int nTempIndex = 0;
        while (true) {
            unsigned char chItemSize = 0;
            fread(&chItemSize, sizeof(chItemSize), 1, fpData);
            if (ferror(fpData)) {//ferror�򷵻�-1
                return -1;
            }
            else if (feof(fpData)) {//����ѭ����ԭ����feof
                break;
            }
            if (chItemSize > 0) {//����0˵�������˴�С����                
                int nFilePos = ftell(fpData);
                if (nFilePos == -1) {
                    return -1;
                }
                else {
                    nNextSavingIndex = nFilePos - 1;//����itemSize�����±�
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

//���±�ΪnIndex�����ÿռ�д��item,�����Ƿ�ɹ�д��
bool WriteItemInDeprecatedIndex(int nIndex, const Item *pItem) {
    int nNextValidIndex = GetFirstValidIndex(nIndex);//Ҫд��������ڴ��±�    
    int nFreeSize = nNextValidIndex - nIndex;//���ٿռ�ɹ�д��
    if (nNextValidIndex == -1) {//-1˵��nIndex֮�󲻴�����Ч���ݣ�����ֱ����nIndex��д��
        g_nSavingIndex = nIndex;//���ں���û����Ч���ݣ�ֱ�Ӹ�ֵ
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fwrite(&g_nSavingIndex, sizeof(g_nSavingIndex), 1, fpData) != 1) {//�����ļ��е�g_nSavingIndex
            exit(EXIT_FAILURE);
        }
        CloseDevice();
        return false;//������һ�㺯��ר�Ŵ�����g_nSavingIndex��д������
    }

    bool bSuccess = true;
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }
    if (fseek(fpData, nIndex, SEEK_SET) != 0) {//��λ
        exit(EXIT_FAILURE);
    }
    int nNewItemSize = pItem->head.chItemSize;
    if (nNewItemSize <= nFreeSize) {
        if (fwrite(pItem, pItem->head.chItemSize, 1, fpData) != 1) {//д��item
            exit(EXIT_FAILURE);
        }

        unsigned char chZero = 0;
        unsigned char chLeftSize = nFreeSize - nNewItemSize;//ʣ����ٿռ�û��

        if (chLeftSize >= 1) {//ֻʣ1�ֽڿ���ռ�����ʱ
            if (fwrite(&chLeftSize, sizeof(chLeftSize), 1, fpData) != 1) {//sizeд���ȥ
                exit(EXIT_FAILURE);
            }
        }
        if (chLeftSize >= 2) {//ֻʣ2�ֽڿ���ռ�����ʱ
            if (fwrite(&chZero, sizeof(chZero), 1, fpData) != 1) {//д��0,��deprecated�ռ�
                exit(EXIT_FAILURE);
            }
        }
    }
    else {
        bSuccess = false;//�ռ䲻�㣬��һ�㺯�������д��
    }
    CloseDevice();
    return bSuccess;    
}

//���سɹ���ӵ�item�����ļ��е��±�
int AddItem(const Item *pItem) {
    int nWriteIndex = GetFirstDeprecatedIndex(BIAS);//Ҫд��������ڴ��±�
    if (nWriteIndex >= BIAS) {//�ҵ������ÿռ�
        if (WriteItemInDeprecatedIndex(nWriteIndex, pItem)) {
            return nWriteIndex;//�ɹ�д���򷵻أ�����ִ�к���д�뷽��
        }        
    }    

    int nIndex = -1;
    if (!OpenDevice()) {
        goto ErrorProc;
    }    

    if (fseek(fpData, g_nSavingIndex, SEEK_SET) != 0) {//дitemǰ���������ļ�ָ�뵽�հ�����
        goto ErrorProc;
    }
    if (fwrite(pItem, pItem->head.chItemSize, 1, fpData) != 1) {//д��item
        goto ErrorProc;
    }
    g_nSavingIndex += pItem->head.chItemSize;//����g_nSavingIndex��������һ��д��

    if (fseek(fpData, 0, SEEK_SET) != 0) {//дg_nSavingIndexǰ���������ļ�ָ�뵽��ͷ
        goto ErrorProc;
    }
    if (fwrite(&g_nSavingIndex, sizeof(g_nSavingIndex), 1, fpData) != 1) {//дg_nSavingIndex
        goto ErrorProc;
    }
    nIndex = g_nSavingIndex - pItem->head.chItemSize;//д���item�����ļ����±�
ErrorProc:
    CloseDevice();
    return nIndex;
}


//���g_itemTable����
//������
//	��
//����ֵ��
//	��
void EmptyItemTable() {
    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        g_itemIndexTable[i] = -1;
    }
}


//Ѱ�ҵ�һ����ʹ��λ�õ��±�,������������￪ʼ����,����ռ���δ����κ���Ч�ַ���������-1
//������
//	int nStartIndex [in]���ĸ��±꿪ʼ����
//����ֵ��
//	int�������ҵ����±꣬û�ҵ�����-1 
int GetFirstValidIndex(int nStartIndex) {
    bool bSuccess = true;
    Item item = { 0 };
    tagHead head = { 0 };

    if (!OpenDevice()) {
        bSuccess = false;
        goto ErrorProc;
    }
    if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//������ǰnStartIndex�ֽ�
        bSuccess = false;
        goto ErrorProc;
    }
    while (nStartIndex < g_nSavingIndex) {
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            bSuccess = false;
            goto ErrorProc;
        }
        if (head.chItemSize > sizeof(Item) && head.bIsUsed) {//����sizeof(Item)����ʹ�ã�������Ч
            break;//�ҵ��ˣ�����ѭ��
        }
        else {
            nStartIndex += head.chItemSize;//����ͷ��Item���ֽ���
            if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//������λ��
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
    if (nStartIndex == g_nSavingIndex) {//���˵��û�и�����
        nStartIndex = -1;
    }
    return nStartIndex;
}

//Ѱ�ҵ�һ��������λ�õ��±�,������������￪ʼ����,����ռ���δ����κ���Ч�ַ���������-1
//������
//	int nStartIndex [in]���ĸ��±꿪ʼ����
//����ֵ��
//	int�������ҵ����±꣬û�ҵ�����-1 
int GetFirstDeprecatedIndex(int nStartIndex) {
    bool bSuccess = true;
    Item item = { 0 };
    tagHead head = { 0 };

    if (!OpenDevice()) {
        bSuccess = false;
        goto ErrorProc;
    }
    if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//������ǰnStartIndex�ֽ�
        bSuccess = false;
        goto ErrorProc;
    }
    while (nStartIndex < g_nSavingIndex) {
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            bSuccess = false;
            goto ErrorProc;
        }
        if (!(head.chItemSize > sizeof(Item) && head.bIsUsed)) {//��Ч������ȡ��
            break;//�ҵ��ˣ�����ѭ��
        }
        else {
            nStartIndex += head.chItemSize;//����ͷ��Item���ֽ���
            if (fseek(fpData, nStartIndex, SEEK_SET) != 0) {//������λ��
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
    if (nStartIndex == g_nSavingIndex) {//���˵��û�и�����
        nStartIndex = -1;
    }
    return nStartIndex;
}

//��ʾ����item��Ϣ
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
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//��λ��׼����ȡvalidIndex��Ӧ��item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//��ȡһ��item
                exit(EXIT_FAILURE);
            }
            printf("�±꣺%x, ������%s, �绰��%llu\n", g_itemIndexTable[i], pItem->szName, pItem->ullPhone);            
        }
    }
    CloseDevice();
    free(pItem);
    pItem = NULL;    
}

//��ȡ����item�����±꣬����g_itemTable��
//������
//	��
//����ֵ��
//	��
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
        if (fseek(fpData, nValidIndex, SEEK_SET) != 0) {//��λ��nValidIndex
            exit(EXIT_FAILURE);
        }
        tagHead head = { 0 };
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            exit(EXIT_FAILURE);
        }
        CloseDevice();
        nValidIndex = GetFirstValidIndex(nValidIndex + head.chItemSize);//��λ��һ�����ܵ��±�
    }
}

//��itemTable�м�鴫���item�Ƿ����
//������
//	int index [in]Ҫ����item���ļ��е��±�
//����ֵ��
//	bool	���ڷ���true�����򷵻�false
bool SearchItem(int nIndex) {
    bool bFound = false;
    GetAllItem();
    for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
        if (g_itemIndexTable[i] == -1) {//-1�������
            break;
        }
        if (g_itemIndexTable[i] == nIndex) {
            bFound = true;
            break;
        }
    }
    return bFound;
}

//�Ƴ�item������true����ɹ�,false����ʧ��
//������
//	Item *pItem [in]Ҫ�Ƴ�item���ļ��е��±�
//����ֵ��
//	bool	����true����ɹ�,false����ʧ��
bool RemoveItem(int nIndex) {
    bool bFound = SearchItem(nIndex);
    if (bFound) {
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fseek(fpData, nIndex + 1, SEEK_SET) != 0) {//+1��λ��isUsed�ֶ�
            exit(EXIT_FAILURE);
        }
        bool isUsed = false;
        if (fwrite(&isUsed, sizeof(isUsed), 1, fpData) != 1) {////д��item
            exit(EXIT_FAILURE);
        }
    }
    CloseDevice();
    return bFound;
}

//�޸�item
//������
//	int index [in]Ҫ�޸��ĸ��±��Ӧ��item
//	const Item *pItem [in]�µ�item
//����ֵ��
//	bool ����true����ɹ�,false����ʧ��
bool ModifyItem(int nIndex, const Item *pItem) {
    if (SearchItem(nIndex)) {
        int nNextValidIndex = 0;
        for (int i = 0; i < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++i) {
            if (g_itemIndexTable[i] == nIndex) {
                nNextValidIndex = g_itemIndexTable[i + 1];
                break;
            }
        }
        int nFreeSize = nNextValidIndex - nIndex;//���ٿռ�ɹ�д��
        if (nNextValidIndex == -1) {//-1˵������index�����һ��validIndex�������ڿ�϶
            nFreeSize = pItem->head.chItemSize;
        }

        bool bSuccess = true;
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fseek(fpData, nIndex, SEEK_SET) != 0) {//��λ
            exit(EXIT_FAILURE);
        }
        int nNewItemSize = pItem->head.chItemSize;
        if (nNewItemSize <= nFreeSize) {
            if (fwrite(pItem, pItem->head.chItemSize, 1, fpData) != 1) {//д��item
                exit(EXIT_FAILURE);
            }

            unsigned char chZero = 0;
            unsigned char chLeftSize = nFreeSize - nNewItemSize;//ʣ����ٿռ�û��

            if (chLeftSize >= 1) {//ֻʣ1�ֽڿ���ռ�����ʱ
                if (fwrite(&chLeftSize, sizeof(chLeftSize), 1, fpData) != 1) {//sizeд���ȥ
                    exit(EXIT_FAILURE);
                }
            }
            if (chLeftSize >= 2) {//ֻʣ2�ֽڿ���ռ�����ʱ
                if (fwrite(&chZero, sizeof(chZero), 1, fpData) != 1) {//д��0,��deprecated�ռ�
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            RemoveItem(nIndex);//�ռ�Ų�����ֱ��ɾ�������
            AddItem(pItem);
        }
        CloseDevice();
        return bSuccess;
    }
    else {
        return false;
    }
}

//��Ƭ����ʹ�ļ���඼����ʹ�ÿռ䣬�Ҳ඼��δʹ�ÿռ�
//������
//	��
//����ֵ��
//	��
void Defragment() {
    int nDeprecatedIndex = GetFirstDeprecatedIndex(BIAS);
    if (nDeprecatedIndex == -1) {//����Ƭ��ֱ�ӷ���
        return;
    }

    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }
    if (fseek(fpData, nDeprecatedIndex, SEEK_SET) != 0) {//��λ
        exit(EXIT_FAILURE);
    }
    tagHead head = { 0 };
    if (fread(&head, sizeof(head), 1, fpData) != 1) {
        exit(EXIT_FAILURE);
    }

    int nValidIndex = GetFirstValidIndex(nDeprecatedIndex + head.chItemSize);
    Item *pItem = NULL;
    while (nDeprecatedIndex >= 0 && nValidIndex > nDeprecatedIndex) {//��������������
        if (!OpenDevice()) {
            exit(EXIT_FAILURE);
        }
        if (fseek(fpData, nValidIndex, SEEK_SET) != 0) {//��λ
            exit(EXIT_FAILURE);
        }
        if (fread(&head, sizeof(head), 1, fpData) != 1) {
            exit(EXIT_FAILURE);
        }
        Item *pTemp = (Item *)realloc(pItem, head.chItemSize);//����pTemp����ֹ����NULL����pItem�������߻�������free
        if (!pTemp) {
            exit(EXIT_FAILURE);
        }
        pItem = pTemp;
        if (fseek(fpData, nValidIndex, SEEK_SET) != 0) {//��λ
            exit(EXIT_FAILURE);
        }
        if (fread(pItem, head.chItemSize, 1, fpData) != 1) {
            exit(EXIT_FAILURE);
        }

        if (fseek(fpData, nDeprecatedIndex, SEEK_SET) != 0) {//��λ
            exit(EXIT_FAILURE);
        }
        if (fwrite(pItem, head.chItemSize, 1, fpData) != 1) {///д��item
            exit(EXIT_FAILURE);
        }
        if (fflush(fpData) == EOF) {
            exit(EXIT_FAILURE);
        }

        nDeprecatedIndex += head.chItemSize;//ǰ��Item�����ֽ�������һ��ת�����ݵ���λ��
        nValidIndex += head.chItemSize;//ǰ��Item�����ֽ�������һ�δӴ�λ�ÿ�ʼ����
        nValidIndex = GetFirstValidIndex(nValidIndex);//��λ��һ����Ч�±�
    }
    free(pItem);
    pItem = NULL;
    g_nSavingIndex = nDeprecatedIndex;//��һ�����ַ����ı����ڴ��±�
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }
    if (fseek(fpData, 0, SEEK_SET) != 0) {//��λ��ͷ��д��g_nSavingIndex
        exit(EXIT_FAILURE);
    }
    if (fwrite(&g_nSavingIndex, sizeof(g_nSavingIndex), 1, fpData) != 1) {//д��g_nSavingIndex
        exit(EXIT_FAILURE);
    }
    CloseDevice();
}



//��ʾ�洢��Ϣ
//������
//	��
//����ֵ��
//	��
void ShowInformation() {
    puts("UUUUUUUU UUUUUUUU");//ǰ0x10�ֽ�����ռ��
    int nValidIndex = GetFirstValidIndex(BIAS);

    GetAllItem();
    int itemCount = 0;
    for (; itemCount < sizeof(g_itemIndexTable) / sizeof(g_itemIndexTable[0]); ++itemCount) {//�����ܸ���
        if (g_itemIndexTable[itemCount] == -1) {
            break;
        }
    }

    //׼������ÿ��validIndex��Ӧ��size��׼��������Ŀռ��д��ÿ��itemռ���ֽ���
    unsigned char *pSizeTable = (unsigned char *)malloc(sizeof(char) * itemCount);
    if (!pSizeTable) {
        exit(EXIT_FAILURE);
    }
    if (!OpenDevice()) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < itemCount; i++) {
        if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//��λ��׼����ȡvalidIndex��Ӧ��size
            exit(EXIT_FAILURE);
        }
        if (fread(pSizeTable + i, sizeof(char), 1, fpData) != 1) {//��ȡvalidIndex��Ӧ��size
            exit(EXIT_FAILURE);
        }
    }

    int nPrintCount = 0;
    for (int i = BIAS; i < g_nSavingIndex; i++) {//g_nSavingIndex��ʼ����δʹ�ÿռ�
        bool bInRange = false;
        for (int j = 0; j < itemCount; j++) {
            if (i >= g_itemIndexTable[j] && i < g_itemIndexTable[j] + pSizeTable[j]) {
                //i�������ļ��е��±꣬���ڷ�Χ�����ӡU
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

//��ʾÿ����ĸ�ĳ��ִ����ͱ���
//������
//	��
//����ֵ��
//	��
void ShowEachCharInformation() {
    int chAlphabet[26 + 6 + 26] = { 0 };//ASCII��Z��a֮����6���ַ�����������ʱֱ�Ӻ�����6���±�
    int nCharCount = 0;//��ĸ����
    int nWordCount = 0;//���ָ���    
    tagWordInfo words[INPUT_LENGTH * 10] = { 0 };
    int index = 0;//words���±�
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
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//��λ��׼����ȡvalidIndex��Ӧ��item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//��ȡһ��item
                exit(EXIT_FAILURE);
            }

            int nStringSize = pItem->head.chItemSize - sizeof(Item);
            for (int j = 0; j < nStringSize - 1; ++j) {//-1ȥ��'\0'

                char ch = pItem->szName[j];//���һ���ַ�����ÿ���ַ�
                if ((unsigned)ch < 128) {
                    if (isalpha(ch)) {
                        ++nCharCount;
                        ++chAlphabet[ch - 'A'];//��дA���±�0��B���±�1��Z���±�25��Сдa���±�32
                    }
                }
                else {
                    ++nWordCount;
                    tagWordInfo info = { 1,(unsigned)pItem->szName[j],(unsigned)pItem->szName[j + 1] };//���ڵ���128��һ�ζ�2��
                    bool bExist = false;
                    for (size_t k = 0; k < sizeof(words) / sizeof(words[0]); k++) {
                        if (words[k].bLowByte == info.bLowByte && words[k].bHighByte == info.bHighByte) {
                            ++words[k].nCount;//�����Ѵ�����ֱ�ӵ�������
                            bExist = true;
                            break;
                        }
                    }
                    if (!bExist) {
                        words[index] = info;//���ֲ�������ֱ�Ӹ�ֵ
                    }
                    ++index;
                    ++j;//����һ�ζ�ȡ�����������Ե���
                }
            }
        }
    }
    free(pItem);
    pItem = NULL;
    if (nCharCount) {
        for (int i = 0; i < 26; ++i) {
            if (chAlphabet[i]) {
                printf("%2c ����%5d�Σ�ռ��%6.2f%%\n", 'A' + i, chAlphabet[i], 100 * (double)chAlphabet[i] / nCharCount);
            }            
        }
        for (int i = 0; i < 26; ++i) {
            if (chAlphabet[i + 26 + 6]) {
                printf("%2c ����%5d�Σ�ռ��%6.2f%%\n", 'a' + i, chAlphabet[i + 26 + 6], 100 * (double)chAlphabet[i + 26 + 6] / nCharCount);
            }            
        }
    }
    if (nWordCount) {
        for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
            if (words[i].bLowByte) {
                printf("%c%c ����%5d�Σ�ռ��%6.2f%%\n", (char)words[i].bLowByte, (char)words[i].bHighByte,
                    words[i].nCount, 100 * (double)words[i].nCount / nWordCount);

            }
        }

    }
}

//��Ѱ�ַ���,֧��ģ������
//������
//	const char *pSubString [in]Ҫƥ����ַ���
//����ֵ��
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
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//��λ��׼����ȡvalidIndex��Ӧ��item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//��ȡһ��item
                exit(EXIT_FAILURE);
            }

            if (strstr((char *)pItem->szName, pSubString)) {
                bFound = true;
                printf("�±꣺%x, ������%s, �绰��%llu\n", g_itemIndexTable[i], pItem->szName, pItem->ullPhone);
            }
        }
    }
    free(pItem);
    pItem = NULL;
    if (!bFound) {
        printf("δ�ҵ�ƥ����\n");
    }
}

//��Ѱ�ַ���,֧��ģ������
//������
//	const char *pSubString [in]Ҫƥ����ַ���
//����ֵ��
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
            if (fseek(fpData, g_itemIndexTable[i], SEEK_SET) != 0) {//��λ��׼����ȡvalidIndex��Ӧ��item
                exit(EXIT_FAILURE);
            }
            if (fread(pItem, sizeof(Item) + INPUT_LENGTH, 1, fpData) != 1) {//��ȡһ��item
                exit(EXIT_FAILURE);
            }
            
            char szBuff[21] = { 0 };
            _ui64toa(pItem->ullPhone, szBuff, 10);
            if (strstr(szBuff, pSubString)) {
                bFound = true;
                printf("�±꣺%x, ������%s, �绰��%llu\n", g_itemIndexTable[i], pItem->szName, pItem->ullPhone);
            }
        }
    }
    free(pItem);
    pItem = NULL;
    if (!bFound) {
        printf("δ�ҵ�ƥ����\n");
    }
}