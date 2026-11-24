# Regression test for #1550: PyViewer.setChannels() must set every channel choice of the viewer, not only
# RGB and Luminance (the channel names were looked up among the menu labels "Red", "Alpha"...).
import NatronEngine
from natrontest import *

Channels = NatronEngine.Natron.DisplayChannelsEnum

v = app.getViewer("Viewer1")
check(v is not None, "no viewer Viewer1")
for c in (Channels.eDisplayChannelsA, Channels.eDisplayChannelsR, Channels.eDisplayChannelsG,
          Channels.eDisplayChannelsB, Channels.eDisplayChannelsY, Channels.eDisplayChannelsMatte,
          Channels.eDisplayChannelsRGB):
    v.setChannels(c)
    wait_idle()
    check(v.getChannels() == c, "setChannels(%s): getChannels() is %s" % (c, v.getChannels()))
