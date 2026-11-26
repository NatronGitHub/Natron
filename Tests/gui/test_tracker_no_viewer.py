"""Regression test for #1256 in the GUI: the buttons of a Tracker that no viewer shows.

The Tracker's buttons did nothing when no viewer showed the Tracker (TrackerNode::knobChanged() returned
at once). The test also checks that the transform solve that follows the tracking ends.
"""

from natrontest import *

CHECKERBOARD = "net.sf.openfx.CheckerBoardPlugin"
# Engine/TrackerContextPrivate.h
AUTO_COMPUTE_TRANSFORM = "autoComputeransform"  # kTrackerParamAutoGenerateTransform (sic)
MOTION_TYPE_MATCH_MOVE = 2  # eTrackerMotionTypeMatchMove
for plugin in (CHECKERBOARD, "net.sf.openfx.TransformPlugin", "net.sf.openfx.CornerPinPlugin"):
    require_plugin(plugin)

checker = app.createNode(CHECKERBOARD)
check(checker is not None, "createNode(%s) failed" % CHECKERBOARD)
trackerNode = app.createNode("fr.inria.built-in.Tracker")
check(trackerNode is not None, "no Tracker node")
trackerNode.connectInput(0, checker)
# the viewer shows the CheckerBoard, not the Tracker
viewer_node(app).connectInput(0, checker)
app.clearSelection()
wait_idle()

track = trackerNode.getTrackerContext().createTrack()
track.getParam("centerPoint").set(256, 256)  # a corner of the 64x64 boxes
# no automatic solve before the tracking: the solve checked below is the one that follows the tracking
trackerNode.getParam(AUTO_COMPUTE_TRANSFORM).set(False)
motionType = trackerNode.getParam("motionType")
motionType.set(MOTION_TYPE_MATCH_MOVE)
app.getViewer("Viewer1").seek(1)
trackerNode.getParam("selectAllMenuAction").trigger()
trackerNode.getParam("trackNext").trigger()

center = track.getParam("centerPoint")
transformCenter = trackerNode.getParam("center")


def solved():
    # the solve sets the center of the transform to the center of the track at the reference frame
    c = transformCenter.get()
    return abs(c.x - 256) < 1e-6 and abs(c.y - 256) < 1e-6


done = wait_until(lambda: center.getKeyIndex(2, 0) >= 0 and solved() and motionType.getIsEnabled(), 30,
                  step=lambda: app.waitForRenders(1))
check(center.getKeyIndex(2, 0) >= 0, "Track Next did not track a Tracker that no viewer shows")
check(solved(), "the transform was not solved after the tracking")
check(motionType.getIsEnabled(), "the solver parameters stayed disabled after the tracking")
check(done, "timeout")
