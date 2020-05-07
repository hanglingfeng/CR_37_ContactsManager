#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define INPUT_LENGTH 99

#pragma pack(push)
#pragma pack(1)
struct tagHead {//ÿ����ϵ�˿�ͷ����
	unsigned char chItemSize;
	bool bIsUsed;

};
#pragma pack(pop)


#pragma pack(push)
#pragma pack(1)
struct tagContact {//��ϵ�˵����ݱ�ʾ
    tagHead head;
    unsigned long long ullPhone;
    char szName[1];
};
#pragma pack(pop)

typedef tagContact Item;


struct tagWordInfo {//���ָ���������
	unsigned nCount : 16;
	unsigned bLowByte : 8;
	unsigned bHighByte : 8;
};

void InitializeIndexData();
int AddItem(const Item *pItem);
int GetFirstValidIndex(int nStartIndex);
int GetFirstDeprecatedIndex(int nStartIndex);
void GetAllItem();
bool RemoveItem(int nIndex);
bool SearchItem(int nIndex);
bool ModifyItem(int nIndex, const Item *pItem);
void Defragment();
void ShowInformation();
void ShowEachCharInformation();
void QueryItemByName(const char *pSubString);
void QueryItemByPhone(const char *pSubString);
void ShowAllItem();