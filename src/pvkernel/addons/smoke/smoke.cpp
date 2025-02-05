//
//  Piano Video
//  A free piano visualizer.
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
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>
#include "../../random.hpp"
#include "../../utils.hpp"

#define  AIR_RESIST  0.95
#define  MAX_AGE     8
#define  DIFF_DIST   4
#define  DIFF_STR    1


struct SmokePtcl {
    SmokePtcl() {
        good = true;
    }

    bool good;  // Whether to read from cache

    float age;  // Seconds

    // x, y are pixel locations.
    // vx, vy are pixel per frame values.
    float x, y, vx, vy;
};


void smoke_read_cache(std::vector<SmokePtcl>& ptcls, std::ifstream& fp) {
    if (!fp.good()) {
        std::cerr << "WARNING: smoke.cpp, smoke_read_cache: Cannot read file." << std::endl;
        return;
    }
    int count;
    fp.read((char*)(&count), sizeof(count));

    for (int i = 0; i < count; i++) {
        SmokePtcl ptcl;
        fp.read((char*)(&ptcl), sizeof(SmokePtcl));
        if (ptcl.good)
            ptcls.push_back(ptcl);
    }
}

void smoke_write_cache(std::vector<SmokePtcl>& ptcls, std::ofstream& fp) {
    if (!fp.good()) {
        std::cerr << "WARNING: smoke.cpp, smoke_write_cache: Cannot write file." << std::endl;
        return;
    }

    const int count = ptcls.size();

    fp.write((char*)(&count), sizeof(count));
    for (int i = 0; i < count; i++) {
        const SmokePtcl& ptcl = ptcls[i];
        fp.write((char*)(&ptcl), sizeof(SmokePtcl));
    }
}


void smoke_sim_diff(SmokePtcl* ptcls, const int size, CD strength) {
    for (int i = 0; i < size-1; i++) {
        for (int j = i+1; j < size; j++) {
            SmokePtcl* p1 = &ptcls[i];
            SmokePtcl* p2 = &ptcls[j];
            CD dx = p1->x - p2->x, dy = p1->y - p2->y;
            CD dist = pythag(dx, dy);

            if (dist <= DIFF_DIST) {
                CD curr_strength = strength * (1-(dist/DIFF_DIST));

                // ddx = delta (delta x) = change in velocity
                CD total_vel = dx + dy;
                CD ddx = curr_strength * (dx/total_vel), ddy = curr_strength * (dy/total_vel);

                p1->vx += ddx;
                p1->vy += ddy;
                p2->vx -= ddx;
                p2->vy -= ddy;
            }
        }
    }
}


extern "C" void smoke_sim(CD fps, const int frame, const int num_new, const int num_notes,
        CD* const x_starts, CD* const x_ends, CD y_start, CD x_vel_min, CD x_vel_max,
        CD y_vel_min, CD y_vel_max, const char* const ip, const char* const op,
        const int width, const int height, const bool diffusion) {
    /*
    Simulate one frame of smoke activity.

    :param fps: Video fps.
    :param num_new: Number of new particles to generate.
    :param num_notes: Number of notes that are playing.
    :param x_starts, x_ends: X coordinate boundaries for each note.
    :param y_start: Y coordinate.
    :param x_vel_min, x_vel_max, y_vel_min, y_vel_max: Pixels per second bounds.
    :param ip: Input file path (leave blank if no input).
    :param op: Output file path.
    */

    CD vx_min = x_vel_min/fps, vx_max = x_vel_max/fps;
    CD vy_min = y_vel_min/fps, vy_max = y_vel_max/fps;

    std::vector<SmokePtcl> ptcls;
    ptcls.reserve((int)1e6);

    // Read from input file
    if (strlen(ip) > 0) {
        std::ifstream fin(ip);
        smoke_read_cache(ptcls, fin);
    }

    // Add new particles
    for (int i = 0; i < num_notes; i++) {
        // Add a bit of jitter to the emission
        // so looks more random.
        CD start = x_starts[i], end = x_ends[i];
        CD x_size = end - start;

        CD phase = sin(i+frame/10.0);
        CD gap = (phase+1)/2.0 * (x_size/2.0);

        CD real_start = start + gap;
        CD real_end = start + gap + x_size/2.0;
        CD real_vmin = vx_min + phase/5.0;
        CD real_vmax = vx_max + phase/5.0;

        for (int j = 0; j < num_new; j++) {
            SmokePtcl ptcl;
            ptcl.x = Random::uniform(real_start, real_end);
            ptcl.y = y_start;
            ptcl.vx = Random::uniform(real_vmin, real_vmax);
            ptcl.vy = Random::uniform(vy_min, vy_max);
            ptcls.push_back(ptcl);
        }
    }

    const int size = ptcls.size();
    CD air_resist = std::pow(AIR_RESIST, 1/fps);

    // Simulate motion
    for (int i = 0; i < size; i++) {
        SmokePtcl& ptcl = ptcls[i];
        ptcl.x += ptcl.vx;
        ptcl.y += ptcl.vy;
        if (!img_bounds(width, height, ptcl.x, ptcl.y)) {
            ptcl.good = false;
            continue;
        }
        if (ptcl.age > MAX_AGE) {
            ptcl.good = false;
            continue;
        }
        ptcl.vx *= air_resist;
        ptcl.vy *= air_resist;
        ptcl.age += 1/fps;
    }
    if (diffusion)
        smoke_sim_diff(&(ptcls[0]), size, DIFF_STR/fps);

    // Write to output
    std::ofstream fout(op);
    smoke_write_cache(ptcls, fout);
}


extern "C" void smoke_render(UCH* img, const int width, const int height,
        const char* const path, CD intensity) {
    /*
    Render smoke on the image.

    :param path: Input cache path.
    :param intensity: Intensity multiplier.
    */

    std::ifstream fp(path);
    std::vector<SmokePtcl> ptcls;
    ptcls.reserve((int)1e6);
    smoke_read_cache(ptcls, fp);

    const int size = ptcls.size();

    for (int i = 0; i < size; i++) {
        const int x = (int)ptcls[i].x, y = (int)ptcls[i].y;

        if (ptcls[i].age < MAX_AGE && img_bounds(width, height, x, y)) {
            // Use an inverse quadratic interp to make it fade slowly, and suddenly go away.
            const UCH value = 255 * (1-pow(ptcls[i].age/MAX_AGE, 2));
            const UCH white[3] = {value, value, value};

            UCH original[3], modified[3];
            img_getc(img, width, x, y, original);
            img_mix(modified, original, white, intensity/10.0);
            img_setc(img, width, x, y, modified);

            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    const int nx = x+dx, ny = y+dy;
                    if (img_bounds(width, height, nx, ny)) {
                        UCH original[3], modified[3];
                        img_getc(img, width, nx, ny, original);
                        img_mix(modified, original, white, intensity/30.0);
                        img_setc(img, width, nx, ny, modified);
                    }
                }
            }
        }
    }
}
