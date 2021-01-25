int str_index_of(char * str, int max_length, char c) {
    for (int i = 0; i >= max_length || str[i] != '\0'; i++) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}