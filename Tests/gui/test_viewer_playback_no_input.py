"""Regression test for #1652: the playback of a viewer without input plays its frame range.

The viewer's render thread cleared the viewer but didn't give the frame to the playback scheduler, which
waited for it forever: the playback never ended, and the timeline stayed at the first frame.
"""
import NatronEngine
import NatronGuiTest
from natrontest import *

viewer = viewer_node(app)
for i in range(viewer.getMaxInputCount()):
    viewer.disconnectInput(i)
wait_idle()
v = app.getViewer("Viewer1")
check(v is not None, "no viewer tab for Viewer1")
v.setFrameRange(1, 10)
v.setPlaybackMode(NatronEngine.Natron.PlaybackModeEnum.ePlaybackModeOnce)
v.seek(1)
v.startForward()
try:
    ended = app.waitForRenders(30)
    frame = v.getCurrentFrame()
finally:
    # stop a playback that didn't end, so that Natron can quit
    v.pause()
    NatronGuiTest.waitIdle(30)
check(ended, "the playback of a viewer without input never ended (#1652)")
check(frame == 10, "the playback of a viewer without input stopped at frame %d, not 10" % frame)
