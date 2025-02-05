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
#include <fstream>
#include <cstring>
#include <cmath>
#include <vector>
#include "../../utils.hpp"
#include "../../random.hpp"

#define  AIR_RESIST  0.95
#define  MAX_AGE     4
#define  ATTR_DIST   4
#define  ATTR_STR    4

#define  VX_MIN  -10
#define  VX_MAX  10
#define  VY_MIN  -125
#define  VY_MAX  -100


struct Particle {
    Particle() {
        good = true;
    }

    bool good;  // Whether to read from cache

    float age;  // Seconds

    // x, y are pixel locations.
    // vx, vy are pixel per frame values.
    float x, y, vx, vy;
};


void ptcl_read_cache(std::vector<Particle>& ptcls, std::ifstream& fp) {
    if (!fp.good()) {
        std::cerr << "WARNING: particles.cpp, ptcl_read_cache: Cannot read file." << std::endl;
        return;
    }
    int count;
    fp.read((char*)(&count), sizeof(count));

    for (int i = 0; i < count; i++) {
        Particle ptcl;
        fp.read((char*)(&ptcl), sizeof(Particle));
        if (ptcl.good)
            ptcls.push_back(ptcl);
    }
}

void ptcl_write_cache(std::vector<Particle>& ptcls, std::ofstream& fp) {
    if (!fp.good()) {
        std::cerr << "WARNING: particles.cpp, ptcl_write_cache: Cannot write file." << std::endl;
        return;
    }

    const int count = ptcls.size();

    fp.write((char*)(&count), sizeof(count));
    for (int i = 0; i < count; i++) {
        const Particle& ptcl = ptcls[i];
        fp.write((char*)(&ptcl), sizeof(Particle));
    }
}


extern "C" void ptcl_sim(CD fps, const int frame, const int num_new, const int num_notes,
        CD* x_starts, CD* x_ends, CD y_start, const char* ip, const char* op, const int width,
        const int height) {
    /*
    Simulate one frame of smoke activity.

    :param fps: Video fps.
    :param num_new: Number of new particles to generate.
    :param num_notes: Number of notes that are playing.
    :param x_starts, x_ends: X coordinate boundaries for each note.
    :param y_start: Y coordinate.
    :param ip: Input file path (leave blank if no input).
    :param op: Output file path.
    */

    CD vx_min = VX_MIN/fps, vx_max = VX_MAX/fps;
    CD vy_min = VY_MIN/fps, vy_max = VY_MAX/fps;

    std::vector<Particle> ptcls;
    ptcls.reserve((int)1e4);

    // Read from input file
    if (strlen(ip) > 0) {
        std::ifstream fin(ip);
        ptcl_read_cache(ptcls, fin);
    }

    // Add new particles
    for (int i = 0; i < num_notes; i++) {
        // Add a bit of jitter to the emission
        // so looks more random.
        CD start = x_starts[i], end = x_ends[i];
        CD x_size = end - start;

        CD phase = sin(12+i+1.2*frame/10.0);
        CD gap = (phase+1)/2.0 * (x_size/2.0);

        CD real_start = start + gap;
        CD real_end = start + gap + x_size/2.0;
        CD real_vmin = vx_min + phase/5.0;
        CD real_vmax = vx_max + phase/5.0;

        for (int j = 0; j < num_new; j++) {
            Particle ptcl;
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
        Particle& ptcl = ptcls[i];
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

    // Simulate attracted to each other
    for (int i = 0; i < size-1; i++) {
        for (int j = i+1; j < size; j++) {
            Particle* p1 = &ptcls[i];
            Particle* p2 = &ptcls[j];
            CD dx = p1->x - p2->x, dy = p1->y - p2->y;
            CD dist = pythag(dx, dy);

            if ((dist <= ATTR_DIST) && (abs(dx+dy) >= 1)) {
                CD curr_strength = ATTR_STR/fps * (1-(dist/ATTR_DIST));

                // ddx = delta (delta x) = change in velocity
                CD total_vel = dx + dy;
                CD ddx = curr_strength * (dx/total_vel), ddy = curr_strength * (dy/total_vel);

                p1->vx -= ddx;
                p1->vy -= ddy;
                p2->vx += ddx;
                p2->vy += ddy;
            }
        }
    }

    // Write to output
    std::ofstream fout(op);
    ptcl_write_cache(ptcls, fout);
}


extern "C" void ptcl_render(UCH* img, const int width, const int height,
        const char* const path, CD intensity) {
    /*
    Render smoke on the image.

    :param path: Input cache path.
    :param intensity: Intensity multiplier.
    */

    std::ifstream fp(path);
    std::vector<Particle> ptcls;
    ptcls.reserve((int)1e4);
    ptcl_read_cache(ptcls, fp);

    const int size = ptcls.size();

    for (int i = 0; i < size; i++) {
        const int x = (int)ptcls[i].x, y = (int)ptcls[i].y;

        if (ptcls[i].age < MAX_AGE && img_bounds(width, height, x, y)) {
            // Use an inverse quadratic interp to make it fade slowly, and suddenly go away.
            const UCH value = 255 * (1-pow(ptcls[i].age/MAX_AGE, 2));
            const UCH white[3] = {value, value, value};

            UCH original[3], modified[3];
            img_getc(img, width, x, y, original);
            img_mix(modified, original, white, intensity);
            img_setc(img, width, x, y, modified);

            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    const int nx = x+dx, ny = y+dy;
                    if (img_bounds(width, height, nx, ny)) {
                        UCH original[3], modified[3];
                        img_getc(img, width, nx, ny, original);
                        img_mix(modified, original, white, intensity/3.0);
                        img_setc(img, width, nx, ny, modified);
                    }
                }
            }
        }
    }
}
