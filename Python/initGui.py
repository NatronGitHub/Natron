#Graphical user interface initialisation script for Natron. This script is run only for interactive sessions.
# -*- coding: utf-8 -*-

#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import natron

class StreamCatcher:
    def __init__(self):
        self.value = ''
    def write(self,txt):
        self.value += txt
    def clear(self):
        self.value = ''
catchOut = StreamCatcher()
catchErr = StreamCatcher()
sys.stdout = catchOut
sys.stderr = catchErr