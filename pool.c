#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char **ReadFileAsList(const char *path, int *pLineCount) {
	FILE *fp = fopen(path, "r");
	int nLines = 0;
	
	//count newlines
	int c;
	do {
		c = fgetc(fp);
		if (c == '\n' || c == EOF) nLines++;
	} while (c != EOF);
	
	//allocate line buffer
	char **lineArray = (char **) calloc(nLines, sizeof(char *));
	char linebuf[1024] = { 0 };
	int linelen = 0;
	
	int lineno = 0;
	fseek(fp, 0, SEEK_SET);
	do {
		c = fgetc(fp);
		if (c == '\n' || c == EOF) {
			char *alloc = (char *) calloc(linelen + 1, 1);
			memcpy(alloc, linebuf, linelen + 1);
			memset(linebuf, 0, sizeof(linebuf));
			linelen = 0;
			lineArray[lineno++] = alloc;
		} else {
			linebuf[linelen++] = c;
		}
	} while (c != EOF);
	*pLineCount = lineno;
	
	fclose(fp);
	return lineArray;
}

int SortLineComparator(const void *s1, const void *s2) {
	return strlen(*(const char **) s2) - strlen(*(const char **) s1);
}

void SortLineArray(char **lines, int nLines) {
	qsort(lines, nLines, sizeof(lines[0]), SortLineComparator);
}

char *StringConcat(const char *s1, int len1, const char *s2, int len2) {
	int len = len1 + len2;
	char *buf = (char *) malloc(len + 1);
	memcpy(buf, s1, len1);
	memcpy(buf + len1, s2, len2);
	buf[len1 + len2] = '\0';
	return buf;
}

int StringContains(const char *str, const char *substr) {
	int len = strlen(str);
	int len2 = strlen(substr);
	for (int i = 0; (i < len) && (i + len2 <= len); i++) {
		char c = str[i];
		if (c == substr[0]) {
			if (memcmp(str + i, substr, len2) == 0)
				return 1;
		}
	}
	return 0;
}

int StringStartsWith(const char *string, int len, const char *substring, int lensub) {
	if (lensub > len)
		return 0;
	
	for (int i = 0; i < lensub; i++) {
		if (substring[i] != string[i])
			return 0;
	}
	return 1;
}

int IsPartialSubstring(const char *string1, int len1, const char *string2, int len2, int *outIndex) {
	if (len1 == len2 && memcmp(string1, string2, len1) == 0) return 0;
	
	int found = 0;
	int firstMatchIn1 = -1;
	for (int i = 0; i < len1; i++) {
		int taillen = i + 1;
		const char *tail = string1 + (len1 - (i + 1));
		if (StringStartsWith(string2, len2, tail, taillen)) {
			firstMatchIn1 = len1 - 1 - i;
			found = 1;
		}
	}
	
	//test if string1 is overlapping the end of string2
	int firstMatchIn2 = -1;
	for (int i = 0; i < len2; i++) {
		int taillen = i + 1;
		const char *tail = string2 + (len2 - (i + 1));
		if (StringStartsWith(string1, len1, tail, taillen)) {
			firstMatchIn2 = len2 - 1 - i;
			found = 1;
		}
	}
	
	//if no matches...
	if (firstMatchIn1 == -1 && firstMatchIn2 == -1)
		return 0;
	
	if (firstMatchIn1 != -1 && firstMatchIn2 != -1) {
		//both matched, which is longer?
		if ((len1 - firstMatchIn1) >= (len2 - firstMatchIn2))
			*outIndex = firstMatchIn1;
		else
			*outIndex = -firstMatchIn2;
	} else if (firstMatchIn1 != -1) {
		*outIndex = firstMatchIn1;
	} else if (firstMatchIn2 != -1) {
		*outIndex = -firstMatchIn2;
	}
	return 1;
}

char *BuildPool(char **strings, int nStrings) {
	SortLineArray(strings, nStrings);
	
	int destIndex = 0;
	char **stringsSrc = (char **) calloc(nStrings, sizeof(char **));
	int *lengths = (int *) calloc(nStrings, sizeof(int));
	for (int i = 0; i < nStrings; i++) {
		char *str = strings[i];
		
		//check for an existing entry that contains this string
		int contained = 0;
		for (int j = 0; j < destIndex; j++) {
			char *str2 = stringsSrc[j];
			if (StringContains(str2, str)) {
				contained = 1;
				break;
			}
		}
		if (contained) continue;
		
		//add to list
		lengths[destIndex] = strlen(str);
		stringsSrc[destIndex++] = str;
	}
	nStrings = destIndex;
	
	//test combinations
	int nStringsSrc = nStrings;
	while (nStrings > 1) {
		int best1 = -1, best2 = -1, bestLength = -1, bestDirection = 1;
		
		//test combinations (i, j)
		for (int i = 0; i < nStrings; i++) {
			char *string1 = stringsSrc[i];
			int len1 = lengths[i];
			
			for (int j = 0; j < nStrings; j++) {
				if (i == j) continue;
				
				//get string1 and string2
				int index = 0;
				char *string2 = stringsSrc[j];
				int len2 = lengths[j];
				if (!IsPartialSubstring(string1, len1, string2, len2, &index)) continue;
				
				//parse out direction and magnitude
				int direction = 1;
				int length;
				if (index < 0) {
					length = len2 + index;
					direction = -1;
				} else {
					length = len1 - index;
				}
					
				if (length > bestLength) {
					bestLength = length;
					bestDirection = direction;
					best1 = i;
					best2 = j;
				}
			}
		}
		
		//if no match found, select two to combine
		if (best1 == -1 || best2 == -1) {
			best1 = 0;
			best2 = 1;
			bestDirection = 1;
			bestLength = 0;
		}
		
		//combine
		char *string1 = stringsSrc[best1];
		char *string2 = stringsSrc[best2];
		int newlen = 0;
		if (bestDirection == 1) {
			stringsSrc[best1] = StringConcat(string1, lengths[best1], string2 + bestLength, lengths[best2] - bestLength);
			newlen = lengths[best1] + lengths[best2] - bestLength;
		} else {
			stringsSrc[best1] = StringConcat(string2, lengths[best2] - bestLength, string1, lengths[best1]);
			newlen = lengths[best2] - bestLength + lengths[best1];
		}
		lengths[best1] = newlen;
		free(string1);
		free(string2);
		
		//delete best2
		memmove(stringsSrc + best2, stringsSrc + best2 + 1, (nStrings - best2 - 1) * sizeof(char *));
		memmove(lengths + best2, lengths + best2 + 1, (nStrings - best2 - 1) * sizeof(int));
		nStrings--;
	}
	
	char *stringBuffer = stringsSrc[0];
	free(stringsSrc);
	free(lengths);
	return stringBuffer;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		puts("Usage: pool <file name>");
		return 1;
	}
	char *path = argv[1];
	
	int nLines = 0;
	char **lines = ReadFileAsList(argv[1], &nLines);
	char *pool = BuildPool(lines, nLines);
	puts(pool);
	
	printf("Pool length: %d\n", strlen(pool));
	
	puts(".");
	return 0;
}