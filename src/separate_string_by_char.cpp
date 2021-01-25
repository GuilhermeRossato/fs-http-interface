#include <windows.h>
#include <stdint.h>

int separate_string_by_char(
	char * str,
	int64_t str_size,
	char separator,
	/* OUT */ char * left,
	int64_t max_left_size,
	/* OUT */ char * right,
	int64_t max_right_size
) {
	int has_separator = 0;
	int separator_id = -1;
	for (int i = 0; i <= str_size; i++) {
		int is_eoq = (i == str_size) || (str[i] == '\0');
		if (is_eoq) {
			break;
		}
		if (str[i] == separator) {
			has_separator = 1;
			separator_id = i;
			break;
		}
	}
	if (!has_separator) {
		if (0 < max_right_size) {
			right[0] = '\0';
		}
		for (int i = 0; i <= str_size; i++) {
			int is_eoq = (i == str_size) || (str[i] == '\0');
			if (is_eoq) {
				if (i < max_left_size) {
					left[i] = '\0';
				}
				break;
			}
			if (i < max_left_size) {
				left[i] = str[i];
			}
		}
		return 1;
	}

	ASSERT("separator id must be in range", separator_id >= 0 && separator_id < str_size);
	ASSERT("there must be a '?' separatoracter in the separator id", str[separator_id] == separator);

	int j = 0;
	for (j = 0; j < separator_id; j++) {
		if (j < max_left_size) {
			left[j] = str[j];
		}
	}
	if (j < max_left_size) {
		left[j] = '\0';
	}
	j++;
	int k = 0;
	for (; j <= str_size; j++) {
		int is_eol = (
			(j == str_size) ||
			(str[j] == '\0')
		);
		if (is_eol) {
			if (k < max_right_size) {
				right[k] = '\0';
			}
			k++;
			break;
		}
		if (k < max_right_size) {
			right[k] = str[j];
		}
		k++;
		continue;
	}
	return 1;
}