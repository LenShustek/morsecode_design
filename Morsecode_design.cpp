/* Morsecode_design.cpp

Convert text into Morse code and generate AutoCAD script files that distributes the
pattern onto one or more subpanels of a space, with allowances for restricted areas.

To use the output from AutoCAD, use the "script" command and point to the <name>.scr file(s)
that this program creates.

To print as a PDF, print, landscape, "Microsoft print to PDF", plot area "window" and define area with borders

L. Shustek, Jan 9, 2021

*/
/*------------- Copyright(c) 2021,2022 Len Shustek ------------------------------------------
The MIT License(MIT)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files(the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------------- */

#include "stdafx.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#define make const float

const char *text = // no punctuation or special characters, except '.' to force stop
   "San Francisco open your Golden Gate "
   "You let no stranger wait outside your door "
   "San Francisco here is your wandering one "
   "Saying Ill wander no more "
   "Other places only make me love you best "
   "Tell me youre the heart of all the golden west "
   "San Francisco welcome me home. again" // <---- forced stop
   "Im coming home to go roaming no more";

#define DETAILS 0  // print details?

#if 0 // right panel only
make panel_width = 40, panel_height = 100;
struct { // restricted areas
   float ll_x, ll_y; // lower left corner
   float ur_x, ur_y; // upper right corner
} restrictions[] = {
   { 0, 39.5, 10, 47 },
   { -1 } };
#elif 1 // both panels

make panel_width = 60, panel_height = 100; // the whole double panel security gate
struct { // define two subpanels
   const char *name;
   float ll_x, ll_y;    // lower left corner
   float ur_x, ur_y;    // upper right corner
   bool right_justify;  // right justify the Morse codes on this subpanel?
} subpanels[] = {
   {"left_panel", 0,0, 19.5, 100, true },       // small left panel, with text right justified
   {"right_panel", 20.5, 0, 60, 100, false },   // big right panel, with text left justified
   {"" } };
struct { // define restricted areas that get no Morse code
   float ll_x, ll_y; // lower left corner
   float ur_x, ur_y; // upper right corner
   bool draw_cutout; // should a cutout be drawn for the restricted area inside the subpanel?
} restrictions[] = {
   { 18,0, 22,100, false },           // a separation between the panels to allow for the frames
   { 20.5,31.0, 20.5+7.5,31.0+10, false },   // space for the lockset on right panel
   { 1,50, 1+6.25,50+9.75, false },   // space for the intercom on left panel
   { -1 } };
#endif

make top_border = 1, bottom_border = 1, left_border = 1, right_border = 1;
make restricted_border = 0.50;

make slot_height = 1.5, slot_vertical_spacing = 0.5;
#define TIMEUNIT 2
make dottime = 1*TIMEUNIT, dashtime = 2*TIMEUNIT, dotdash_space = 0.25*TIMEUNIT, letter_space = 0.5*TIMEUNIT, word_space = 1*TIMEUNIT;
#define DRAW_CMD "RECTANG f 0.25"  // rectangle having round filleted inside corners with a radius of 1/4"

#define MAX_SLOTS 2000
struct { // remembered slots
   float ll_x, ll_y; // lower left corner
   float ur_x, ur_y; // upper right corner
} slots [MAX_SLOTS] = { 0 };

const char *letter_code[26] = { // A..Z
   ".-","-...","-.-.","-..",".","..-.","--.","....",
   "..",".---","-.-",".-..","--","-.","---",".--.",
   "--.-",".-.","...","-","..-","...-",".--","-..-",
   "-.--","--.." };
const char *digit_code[10] = { // 0..9
   "-----",".----","..---","...--","....-",
   ".....","-....","--...","---..","----." };
const char *blank = { "" };

void err(const char *msg, const char *info) {
   printf("Error: %s: %s\n", msg, info);
   exit(8); }
void err(const char *msg, char badch) {
   printf("Error: %s: %c\n", msg, badch);
   exit(8); }

const char *code(char ch) { // find the Morse code for a letter or number
   const char *ptr = NULL;
   ch = toupper(ch);
   if (isdigit(ch)) ptr = digit_code[ch - '0'];
   else if (isalpha(ch)) ptr = letter_code[ch - 'A'];
   else if (ch == ' ') ptr = blank;
   else err("bad letter", ch);
   return ptr; }

float character_width(const char *string) { // compute the width of the Morse code for a glyph
   float w = 0;
   while (char ch = *string++) // space returns zero
      if (ch == '.') w += dottime + dotdash_space;
      else if (ch == '-') w += dashtime + dotdash_space;
      else err("bad code", ch);
   return w; }

// figure out by how much to adjust the x of the lower left corner to avoid restrictions
float avoid_restrictions(float x, float width, float y, float height) {
   for (int i = 0; restrictions[i].ll_x >= 0; ++i)  // look at each restricted area
      if (
         x < restrictions[i].ur_x && x + width > restrictions[i].ll_x
         &&
         y < restrictions[i].ur_y && y+height > restrictions[i].ll_y) {
         if (DETAILS) printf("restricted rectangle at %.2f, %.2f with height %.2f and width %.2f; adjust x by %.2f\n",
                                x, y, height, width, restrictions[i].ur_x - x);
         return restrictions[i].ur_x - x; }
   return 0.0; };

// right adjust the slots of one row in any panel that calls for right justification
void right_adjust(int startslot, int endslot) {
   if (DETAILS) printf("right adjust for slots %d to %d\n", startslot, endslot);
   for (int subpanel = 0; *subpanels[subpanel].name; ++subpanel) { // for each subpanel
      if (subpanels[subpanel].right_justify) { // it is a right-adjust subpanel
         float adjustment = -1;
         for (int slot = endslot; slot >= startslot; --slot) { // look at all the slots in this row, from the right
            if ( // this slot overlaps this subpanel at all
               slots[slot].ll_x < subpanels[subpanel].ur_x && slots[slot].ur_x > subpanels[subpanel].ll_x
               && slots[slot].ll_y < subpanels[subpanel].ur_y && slots[slot].ur_y > subpanels[subpanel].ll_y) {
               if (adjustment < 0) { // if it's the first slot we've seen for this subpanel
                  adjustment = subpanels[subpanel].ur_x - right_border - slots[slot].ur_x; // compute shift amount
                  if (adjustment < 0) adjustment = 0;
                  if (DETAILS) printf("shift slots in panel %d starting with %.2f, %.2f by %.2f to the right\n", subpanel+1, slots[slot].ll_x, slots[slot].ll_y, adjustment); }
               slots[slot].ll_x += adjustment; // slide the slot to the right
               slots[slot].ur_x += adjustment; } } } } }

int main() {
   // first, compute and save all the slots for the entire panel, which is perhaps composed of multiple subpanels
   const char *textptr = text;
   float area = 0;
   int num_slots = 0;
   for (float y = panel_height - top_border - slot_height; // for all rows
         y >= bottom_border;
         y -= slot_height + slot_vertical_spacing) {
      float x = left_border;
      int rowstart_slot = num_slots;
      while(1) { // for all characters on this row
         if (*textptr == '.') { // if we hit a period, it's a forced stop
            right_adjust(rowstart_slot, num_slots - 1); // but first: right adjust this row's slots in any panel that calls for it
            break; }
         float width = character_width(code(*textptr)); // (space returns 0)
         float adjustment;
         do {
            adjustment = avoid_restrictions(x, width, y, slot_height);  // any adjustment needed to avoid restricted areas?
            x += adjustment; }
         while (adjustment != 0);   // (might be several consecutive adjustments as it runs into new restricted areas)
         if (x + width > panel_width - right_border) { // no room for next char: need to go to next row
            right_adjust(rowstart_slot, num_slots-1); // but first: right adjust this row's slots in any panel that calls for it
            break; }
         if (DETAILS) printf("'%c', code %s width %.2f, at %.2f, %.2f\n", *textptr, code(*textptr), width, x, y);
         else printf("%c", *textptr);
         if (*textptr != ' ' || x > left_border) { // ignore blanks at the left edge
            for (const char *codeptr = code(*textptr); *codeptr; ++codeptr) { // for all the dot-dashes of a character
               float slot_width;
               if (*codeptr == '.') slot_width = dottime;
               else if (*codeptr == '-') slot_width = dashtime;
               else err("bad code", codeptr);
               //fprintf(outf, DRAW_CMD " %.2f,%.2f %.2f,%.2f\n", x, y, x + slot_width, y + slot_height);
               slots[num_slots].ll_x = x; slots[num_slots].ll_y = y; // record the slot
               slots[num_slots].ur_x = x + slot_width;
               slots[num_slots].ur_y = y + slot_height;
               if (++num_slots >= MAX_SLOTS) err("too many slots", ' ');
               area += slot_width * slot_height;
               x += slot_width + dotdash_space; }
            x += letter_space; }
         if (!*++textptr) {
            textptr = text; // start over if we run out of text
            if (!DETAILS) printf("\n"); } } // while chars
      if (*textptr == '.') break; // if it was a forced stop
   } // for rows
   printf("\n%.1f%% of the panel space is open, not including restricted areas\n", 100 * area / (panel_width*panel_height));

   // Now output a separate AutoCAD script file for the slots in each subpanel
   for (int subpanel = 0; *subpanels[subpanel].name; ++subpanel) { // for each subpanel
      FILE *outf;
      char filename[80];
      strcpy_s(filename, sizeof(filename), subpanels[subpanel].name);
      strcat_s(filename, sizeof(filename), ".scr");
      fopen_s(&outf, filename, "w");
      fprintf(outf, "SNAPMODE 0\n"); // Or else drawing commands are messed up! That (ugh) cost me 3 hours of hairpulling...
      fprintf(outf, "OSNAPCOORD 1\n"); // (There's no direct way to set OSNAP off with a command, but this works too.)
      fprintf(outf, DRAW_CMD " %.2f,%.2f %.2f,%.2f\n",  // output the bounding box for the subpanel
              subpanels[subpanel].ll_x, subpanels[subpanel].ll_y, subpanels[subpanel].ur_x, subpanels[subpanel].ur_y);
      int subpanel_slots = 0;
      for (int slot = 0; slot < num_slots; ++slot)  // for each slot
         if ( // does this slot overlap this subpanel at all
            slots[slot].ll_x < subpanels[subpanel].ur_x && slots[slot].ur_x > subpanels[subpanel].ll_x
            && slots[slot].ll_y < subpanels[subpanel].ur_y && slots[slot].ur_y > subpanels[subpanel].ll_y) {
            fprintf(outf, DRAW_CMD " %.2f,%.2f %.2f,%.2f\n", // output the slot to the script file
                    slots[slot].ll_x, slots[slot].ll_y, slots[slot].ur_x, slots[slot].ur_y);
            ++subpanel_slots; }
      int subpanel_restrictions = 0;
      for (int i = 0; restrictions[i].ll_x >= 0; ++i) // look at all the restricted areas
         if (restrictions[i].draw_cutout  // draw a cutout, if requested, if this area is entirely inside this subpanel
               && restrictions[i].ll_x >= subpanels[subpanel].ll_x && restrictions[i].ur_x <= subpanels[subpanel].ur_x
               && restrictions[i].ll_y >= subpanels[subpanel].ll_y && restrictions[i].ur_y <= subpanels[subpanel].ur_y) {
            fprintf(outf, DRAW_CMD " %.2f,%.2f %.2f,%.2f\n",
                    restrictions[i].ll_x + restricted_border,
                    restrictions[i].ll_y + restricted_border,
                    restrictions[i].ur_x - restricted_border,
                    restrictions[i].ur_y - restricted_border);
            ++subpanel_restrictions; }
      fclose(outf);
      printf("file %s written with %d Morse code slots and %d restricted area cutout%c\n",
             filename, subpanel_slots, subpanel_restrictions, subpanel_restrictions == 1 ? ' ' : 's'); }
   return 0; }
//*
