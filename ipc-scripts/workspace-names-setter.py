#!/usr/bin/python3

#
# The MIT License (MIT)
#
# Copyright (c) 2025 Scott Moreau <oreaus@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

from wayfire import WayfireSocket
import json
import sys

# Simple script to set workspace names

if len(sys.argv) != 2:
    print("Invalid usage, exactly one argument required: a workspace name for the current output and workspace.")
    exit(-1)

sock = WayfireSocket()

output = sock.get_focused_output()
output_name = output["name"]
current_workspace = str(int(output["workspace"]["x"]) + int(output["workspace"]["y"]) * int(output["workspace"]["grid_width"]) + 1)
json_string = "{\"workspace-names/" + output_name + "_workspace_" + current_workspace + "\": \"" + sys.argv[1] + "\"}"
sock.set_option_values(json.loads(json_string))
