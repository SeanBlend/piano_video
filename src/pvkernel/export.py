#
#  Piano Video
#  A free piano visualizer.
#  Copyright Patrick Huang 2021
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import numpy as np
import cv2
from tqdm import trange
from typing import TYPE_CHECKING
from pv.utils import call_op
from .videoio import VideoWriter

Video = None
if TYPE_CHECKING:
    from .video import Video


def export(context: Video, path: str) -> None:
    """
    Exports the video from a video.
    """
    for op in context.get_jobs("init"):
        call_op(context, op)

    res = context.resolution
    with VideoWriter(path, res, int(context.fps)) as video:
        intro = context.props.core.pause_start * context.fps
        for frame in trange(context.data.core.running_time, desc="Rendering video"):
            context._frame = int(frame - intro)
            context._render_img = np.zeros((res[1], res[0], 3), dtype=np.uint8)

            for op in context.get_jobs("frame_init"):
                call_op(context, op)
            for op in context.get_jobs("frame"):
                call_op(context, op)
            for op in context.get_jobs("frame_deinit"):
                call_op(context, op)
            for op in context.get_jobs("modifiers"):
                call_op(context, op)

            img = cv2.cvtColor(context.render_img, cv2.COLOR_BGR2RGB)
            video.write(img)
