# Regression test for #1652: the playback of a viewer that can't render any frame (here: it has no input)
# must end. The viewer's render thread cleared the viewer but neither gave the frame to the playback
# scheduler nor stopped the playback, so the scheduler waited for that frame forever: the playback never
# ended, and waitForRenders() timed out.
import NatronEngine
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
ended = app.waitForRenders(20)
if not ended:
    v.pause()
    wait_idle()
check(ended, "the playback of a viewer without input never ended (#1652)")
