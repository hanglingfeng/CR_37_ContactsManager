#define _CRT_SECURE_NO_WARNINGS
#include "ContactsManager.h"
#include <stdio.h>
#include <string.h>

void PrintMenu() {
	puts("");
	puts("1.增加联系人        2.删除联系人          3.修改联系人");
	puts("4.查询联系人        5.统计字母和汉字      6.显示存储信息");
	puts("7.显示所有联系人    8.碎片整理");
}

void EatLine() {
	while (getchar() != '\n') {

	}
}

//读一行输入到pBuff里，如果只读取到一个换行符则保留，否则不保留换行符
//参数：
//	const char *pBuff [in]保存字符的缓冲区
//	int nBuffSize [in]保存字符的缓冲区大小
//返回值：
//	无
void ReadLine(char *pBuff, int nBuffSize) {
	fgets(pBuff, nBuffSize, stdin);
	if (pBuff[1] == '\0') {//用户只输入了Enter，不做处理
		return;
	}
	else {
		for (int i = 0; i < nBuffSize; ++i) {
			if (pBuff[i] == '\n') {//不保存读取到的换行符
				pBuff[i] = 0;
				break;
			}
		}
	}
}

void UserAddItem() {
	printf("请输入姓名：");
	char aryBuff[INPUT_LENGTH + 1] = { '\0' };
	ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));

	printf("请输入电话：");
	unsigned long long phoneNumber = 0;
	if (scanf("%llu", &phoneNumber) == 1) {
		size_t count = sizeof(Item) + strlen(aryBuff);
		Item *pItem = (Item *)calloc(count, sizeof(char));//calloc就不用手动处理字符串末尾0了
		if (pItem) {
			//手动构建一个Item
			pItem->head.chItemSize = (unsigned char)count;
			pItem->head.bIsUsed = true;
			pItem->ullPhone = phoneNumber;
			strcpy(pItem->szName, aryBuff);

			int nIndex = AddItem(pItem);
			if (nIndex) {
				printf("联系人保存在下标： %x\n", nIndex);
			}
		}
	}
	else {
		puts("输入错误");
	}
	EatLine();
}

void UserRemoveItem() {
	printf("请输入要删除的下标：");
	int index = 0;
	if (scanf("%x", &index) == 1) {
		if (RemoveItem(index)) {
			puts("删除成功");
		}
		else {
			printf("欲移除的下标无效\n");
		}
	}
	else {
		puts("输入不正确");
	}
	EatLine();
}

void UserModifyItem() {
	int index = 0;

	printf("请输入要修改的下标：");
	if (scanf("%x", &index) == 1) {
		EatLine();

		printf("请输入新姓名：");
		char aryBuff[INPUT_LENGTH + 1] = { '\0' };
		ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));

		printf("请输入新电话：");
		unsigned long long phoneNumber = 0;
		if (scanf("%llu", &phoneNumber) == 1) {
			size_t count = sizeof(Item) + strlen(aryBuff);
			Item *pItem = (Item *)calloc(count, sizeof(char));//calloc就不用手动处理字符串末尾0了
			if (pItem) {
				//手动构建一个Item
				pItem->head.chItemSize = (unsigned char)count;
				pItem->head.bIsUsed = true;
				pItem->ullPhone = phoneNumber;
				strcpy(pItem->szName, aryBuff);

				bool bSuccess = ModifyItem(index, pItem);
				if (bSuccess) {
					puts("修改成功");
				}
				else {
					puts("下标无效");
				}
			}
		}
		else {
			puts("输入错误");
		}
		EatLine();
	}
	else {
		EatLine();
		puts("输入不正确");
	}
}

void UserQuaryItem() {
	puts("1.根据姓名查询        2.根据电话查询");
	printf("请输入序号：");
	int ch = getchar();
	EatLine();

	char aryBuff[INPUT_LENGTH + 1] = { '\0' };
	switch (ch) {
	case '1':
		printf("请输入查询内容,支持模糊匹配：");
		ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));
		QueryItemByName(aryBuff);
		break;
	case '2':
		printf("请输入查询内容,支持模糊匹配：");
		ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));
		QueryItemByPhone(aryBuff);
		break;
	default:
		printf("无此选项\n");
		break;
	}

	/*printf("请输入查询内容,支持模糊匹配：");
	char aryBuff[INPUT_LENGTH + 1] = { '\0' };
	ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));
	QueryItemByName(aryBuff);*/
}

int main() {
	InitializeIndexData();
	int ch = 0;
	while (true) {
		PrintMenu();
		printf("请输入序号：");
		ch = getchar();
		EatLine();

		switch (ch) {
		case '1':
			UserAddItem();
			break;
		case '2':
			UserRemoveItem();
			break;
		case '3':
			UserModifyItem();
			break;
		case '4':
			UserQuaryItem();
			break;
		case '5':
			ShowEachCharInformation();
			break;
		case '6':
			ShowInformation();
			break;		
		case '7':
			ShowAllItem();
			break;
		case '8':
			Defragment();
			puts("整理完毕");
			break;
		default:
			printf("无此选项，请重新输入\n");
			break;
		}
	}
	return 0;
}
