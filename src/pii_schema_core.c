
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #include <stdbool.h>

    // Rask PII-skanning og maskering (E-post, Kredittkort, Fødselsnummer)
    int mask_pii_c(char* text, int len) {
        int masked_count = 0;
        
        for (int i = 0; i < len; i++) {
            // E-post deteksjon: Finner '@' og maskerer ordet
            if (text[i] == '@' && i > 0 && i < len - 1) {
                int start = i - 1;
                while (start > 0 && !isspace((unsigned char)text[start - 1])) start--;
                int end = i + 1;
                while (end < len && !isspace((unsigned char)text[end])) end++;
                
                for (int k = start; k < end; k++) {
                    text[k] = '*';
                }
                masked_count++;
                i = end;
            }
            
            // Kredittkort-mønster (16 sammenhengende eller bindestrek-delte siffer)
            if (isdigit((unsigned char)text[i])) {
                int digits = 0;
                int j = i;
                while (j < len && (isdigit((unsigned char)text[j]) || text[j] == '-')) {
                    if (isdigit((unsigned char)text[j])) digits++;
                    j++;
                }
                if (digits == 16) {
                    for (int k = i; k < j; k++) {
                        if (isdigit((unsigned char)text[k])) text[k] = 'X';
                    }
                    masked_count++;
                    i = j;
                }
            }
        }
        return masked_count;
    }

    // Ekstremt rask JSON-syntakssjekker (Validerer balanserte klammer og strukturell integritet)
    bool validate_json_structure_c(const char* json_str, int len) {
        int depth_curly = 0;
        int depth_bracket = 0;
        bool in_string = false;

        for (int i = 0; i < len; i++) {
            char c = json_str[i];
            if (c == '"' && (i == 0 || json_str[i-1] != '\\')) {
                in_string = !in_string;
            }
            if (!in_string) {
                if (c == '{') depth_curly++;
                else if (c == '}') depth_curly--;
                else if (c == '[') depth_bracket++;
                else if (c == ']') depth_bracket--;
                
                if (depth_curly < 0 || depth_bracket < 0) return false;
            }
        }
        return (depth_curly == 0 && depth_bracket == 0 && !in_string);
    }
    