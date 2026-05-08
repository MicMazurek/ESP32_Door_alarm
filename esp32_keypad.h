/*
 * esp32_keypad.h
 *
 *  Created on: 25 kwi 2026
 *      Author: Rinkazidoshi
 */

#pragma once
#include <stdint.h>
//Reference pins
#define COL_1	15
#define COL_2	16
#define COL_3	17
#define COL_4	18
//IT pins
#define ROW_1 	5
#define ROW_2 	6
#define ROW_3 	7
#define ROW_4 	8

//Keypad size
#define NUM_OF_ROWS		4
#define NUM_OF_COLS		4

//Characters buffer
#define MAX_NUM_OF_CHARACTERS	60
static uint8_t buffer_length=0;
static char buffer[MAX_NUM_OF_CHARACTERS];
char* passwd="1ab";
//Keypad overview
static char keys[NUM_OF_ROWS][NUM_OF_COLS]={
{'1','2','3','a'},
{'4','5','6','b'},
{'7','8','9','c'},
{'#','0','*','d'}
};
int cols[NUM_OF_COLS]={COL_1,COL_2,COL_3,COL_4};
int rows[NUM_OF_ROWS]={ROW_1,ROW_2,ROW_3,ROW_4};

int keypad_init();
int keypad_error();
void keypad_code_action(char* code);
int keypad_set_key(const uint8_t new_key_definition,uint8_t key_col,uint8_t key_row);
int keypad_redefine(const uint8_t new_keys_definition[4][4]);