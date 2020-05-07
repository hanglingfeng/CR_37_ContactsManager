#define _CRT_SECURE_NO_WARNINGS
#include "ContactsManager.h"
#include <stdio.h>
#include <string.h>

void PrintMenu() {
	puts("");
	puts("1.������ϵ��        2.ɾ����ϵ��          3.�޸���ϵ��");
	puts("4.��ѯ��ϵ��        5.ͳ����ĸ�ͺ���      6.��ʾ�洢��Ϣ");
	puts("7.��ʾ������ϵ��    8.��Ƭ����");
}

void EatLine() {
	while (getchar() != '\n') {

	}
}

//��һ�����뵽pBuff����ֻ��ȡ��һ�����з����������򲻱������з�
//������
//	const char *pBuff [in]�����ַ��Ļ�����
//	int nBuffSize [in]�����ַ��Ļ�������С
//����ֵ��
//	��
void ReadLine(char *pBuff, int nBuffSize) {
	fgets(pBuff, nBuffSize, stdin);
	if (pBuff[1] == '\0') {//�û�ֻ������Enter����������
		return;
	}
	else {
		for (int i = 0; i < nBuffSize; ++i) {
			if (pBuff[i] == '\n') {//�������ȡ���Ļ��з�
				pBuff[i] = 0;
				break;
			}
		}
	}
}

void UserAddItem() {
	printf("������������");
	char aryBuff[INPUT_LENGTH + 1] = { '\0' };
	ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));

	printf("������绰��");
	unsigned long long phoneNumber = 0;
	if (scanf("%llu", &phoneNumber) == 1) {
		size_t count = sizeof(Item) + strlen(aryBuff);
		Item *pItem = (Item *)calloc(count, sizeof(char));//calloc�Ͳ����ֶ������ַ���ĩβ0��
		if (pItem) {
			//�ֶ�����һ��Item
			pItem->head.chItemSize = (unsigned char)count;
			pItem->head.bIsUsed = true;
			pItem->ullPhone = phoneNumber;
			strcpy(pItem->szName, aryBuff);

			int nIndex = AddItem(pItem);
			if (nIndex) {
				printf("��ϵ�˱������±꣺ %x\n", nIndex);
			}
		}
	}
	else {
		puts("�������");
	}
	EatLine();
}

void UserRemoveItem() {
	printf("������Ҫɾ�����±꣺");
	int index = 0;
	if (scanf("%x", &index) == 1) {
		if (RemoveItem(index)) {
			puts("ɾ���ɹ�");
		}
		else {
			printf("���Ƴ����±���Ч\n");
		}
	}
	else {
		puts("���벻��ȷ");
	}
	EatLine();
}

void UserModifyItem() {
	int index = 0;

	printf("������Ҫ�޸ĵ��±꣺");
	if (scanf("%x", &index) == 1) {
		EatLine();

		printf("��������������");
		char aryBuff[INPUT_LENGTH + 1] = { '\0' };
		ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));

		printf("�������µ绰��");
		unsigned long long phoneNumber = 0;
		if (scanf("%llu", &phoneNumber) == 1) {
			size_t count = sizeof(Item) + strlen(aryBuff);
			Item *pItem = (Item *)calloc(count, sizeof(char));//calloc�Ͳ����ֶ������ַ���ĩβ0��
			if (pItem) {
				//�ֶ�����һ��Item
				pItem->head.chItemSize = (unsigned char)count;
				pItem->head.bIsUsed = true;
				pItem->ullPhone = phoneNumber;
				strcpy(pItem->szName, aryBuff);

				bool bSuccess = ModifyItem(index, pItem);
				if (bSuccess) {
					puts("�޸ĳɹ�");
				}
				else {
					puts("�±���Ч");
				}
			}
		}
		else {
			puts("�������");
		}
		EatLine();
	}
	else {
		EatLine();
		puts("���벻��ȷ");
	}
}

void UserQuaryItem() {
	puts("1.����������ѯ        2.���ݵ绰��ѯ");
	printf("��������ţ�");
	int ch = getchar();
	EatLine();

	char aryBuff[INPUT_LENGTH + 1] = { '\0' };
	switch (ch) {
	case '1':
		printf("�������ѯ����,֧��ģ��ƥ�䣺");
		ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));
		QueryItemByName(aryBuff);
		break;
	case '2':
		printf("�������ѯ����,֧��ģ��ƥ�䣺");
		ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));
		QueryItemByPhone(aryBuff);
		break;
	default:
		printf("�޴�ѡ��\n");
		break;
	}

	/*printf("�������ѯ����,֧��ģ��ƥ�䣺");
	char aryBuff[INPUT_LENGTH + 1] = { '\0' };
	ReadLine(aryBuff, sizeof(aryBuff) / sizeof(aryBuff[0]));
	QueryItemByName(aryBuff);*/
}

int main() {
	InitializeIndexData();
	int ch = 0;
	while (true) {
		PrintMenu();
		printf("��������ţ�");
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
			puts("�������");
			break;
		default:
			printf("�޴�ѡ�����������\n");
			break;
		}
	}
	return 0;
}
