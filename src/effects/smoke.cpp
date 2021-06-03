//
//  Piano Video
//  Piano MIDI visualizer
//  Copyright Patrick Huang 2021
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <iostream>
#include <fstream>
#include <string>

using std::cout;
using std::cin;
using std::endl;
using std::string;

typedef  unsigned char  UCH;
typedef  unsigned int   UINT;


/*
Pass arguments through stdin.
Don't pass any extra blank lines when using.
They are added here for formatting.

$ ./smoke.out
cache_dir
pathsep

width height fps

num_notes
note1_num note1_x note1_width note1_start_frame note1_end_frame
...

*/


void cache_note(const UINT i, const UINT x_loc, const UINT width, const UINT start, const UINT end) {
    std::ofstream file;
}


int main(const int argc, const char** argv) {
    string cache_dir, pathsep;
    UINT width, height, fps, num_notes;

    getline(cin, cache_dir);
    getline(cin, pathsep);
    cin >> width >> height >> fps >> num_notes;

    UINT** notes = new UINT*[5];
    for (UINT i = 0; i < num_notes; i++) {
        notes[i] = new UINT[5];
        UINT n, x, width, start, end;
        cin >> n >> x >> width >> start >> end;
        notes[i][0] = n;
        notes[i][1] = x;
        notes[i][2] = width;
        notes[i][3] = start;
        notes[i][4] = end;
    }

    for (UINT i = 0; i < num_notes; i++) {
        delete[] notes[i];
    }
    delete[] notes;
}
