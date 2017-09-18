//str.c
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXTOKENS 2000

char* strings[MAXTOKENS];

/*
Brief:
	Splits a null-terminated string on some given delimiter into an array of null-terminated strings on 
Params:	
	str 		> string to split
	delimiter 	> delimiter to split on
Return:
	Array of null-terminated strings 
*/
char** strSplit(char* str, char delimiter) {
	int i = 0;
	int start = 0;
	int end = 0;
	int length = 0;
	while(*(str + end) != '\0') {
		if(str[end] == delimiter) {
			length = end - start;
			strings[i] = malloc(length + 1);
			strncpy(strings[i], str + start, length);
			strings[i + length] = NULL;
			i++;
			start = end + 1;
			end++;
		}
		end ++;
	}
	length = end - start;
	strings[i] = malloc(length + 1);
	strncpy(strings[i], str + start, length);
	strings[i + length] = NULL;
	i++;
	start = end + 1;
	end++;
	strings[i] = NULL;
	return strings;
}

void strTrim(char* str) {
	int leftTrim = 0;
	int rightTrim = 0;
	int i = strlen(str);
	while(str[leftTrim] == ' ' || str[leftTrim] == '\n') {
		leftTrim++;
	}
	for(i = strlen(str) - 1; i >= 0; i--) {
		if(str[i] == ' ' || str[i] == '\n') {
			rightTrim++;
		} else {
			break;
		}
	}
	//trim right
	str[strlen(str) - rightTrim] = '\0';
	//left shift by leftTrim
	for(i = leftTrim; i <= strlen(str); i++) {
		str[i - leftTrim] = str[i];
	}
	i = 0;
}

int strContains(char* src, char* token) {
	return (strstr(src, token) != NULL);
} 