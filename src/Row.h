//
// Created by filip on 20. 08. 2021..
//

#ifndef EDITOR_SRC_ROW_H
#define EDITOR_SRC_ROW_H

#include <cstdio>
#include <cstring>

#include "RawMode.h"
#include "File.h"

extern EditorConfig e;

namespace Row {
	int editor_row_rx_to_cx(erow *row, int rx);
	int editor_row_cx_to_rx(erow *row, int cx);
	void editor_row_insert_char(erow *row, int at, int c);
	void update_row(erow *row);
	void editor_append_row(char *s, size_t len);
	void editor_row_delete_cahr(erow *row, int at);
	void free_row(erow *row);
	void delete_row(int at);
	void append_string_(erow *row, char *s, size_t len);
	void editor_insert_row(int at, char *s, size_t len);
}

#endif //EDITOR_SRC_ROW_H
