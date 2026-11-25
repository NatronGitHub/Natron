"""Effect.getFrameRange() and Effect.getAvailableLayers(inputNb, time, view) (#1799).

Run by NatronRenderer -t (see Tests/CMakeLists.txt). Uses only built-in nodes: the DiskCache node
has a frame range of its own.
"""
import traceback

import NatronEngine
from NatronEngine import natron

INT_MIN = -2 ** 31
INT_MAX = 2 ** 31 - 1


def layer_names(layers):
    """Returns the names of the ImageLayers (ImageLayer == compares identities)."""
    assert isinstance(layers, list), layers
    assert all(isinstance(layer, NatronEngine.ImageLayer) for layer in layers), layers
    return [(layer.getLayerName(), layer.getComponentsPrettyName()) for layer in layers]


def check_layers(node):
    """getAvailableLayers(inputNb) is getAvailableLayers(inputNb, current frame, view 0)."""
    layers = layer_names(node.getAvailableLayers(-1))
    assert layers, "no layer"
    assert layers == layer_names(node.getAvailableLayers(-1, node.getCurrentTime(), 0))
    assert layers == layer_names(node.getAvailableLayers(-1, node.getCurrentTime()))
    assert node.getAvailableLayers(-1, node.getCurrentTime(), -1) == []  # negative view
    return layers


def check_disk_cache(app, dot):
    """A DiskCache has the frame range of its parameters, and the Dot after it too."""
    cache = app.createNode("fr.inria.built-in.DiskCache")
    assert cache is not None, "createNode(DiskCache) failed"
    assert dot.connectInput(0, cache), "connectInput() failed"
    cache.getParam("frameRange").setValue(2)  # Manual
    cache.getParam("firstFrame").setValue(5)
    cache.getParam("LastFrame").setValue(12)
    assert cache.getFrameRange() == (5., 12.), cache.getFrameRange()
    assert dot.getFrameRange() == (5., 12.), dot.getFrameRange()
    cache.getParam("frameRange").setValue(1)  # Project frame range
    app.getProjectParam("frameRange").set(3, 30)
    assert cache.getFrameRange() == (3., 30.), cache.getFrameRange()
    assert dot.getFrameRange() == (3., 30.), dot.getFrameRange()
    check_layers(dot)
    # the input of the Dot has the color layer of the DiskCache (no project layers: not an output)
    assert layer_names(dot.getAvailableLayers(0, 7., 0))[0] == check_layers(cache)[0] == ("Color", "RGBA")


def main():
    status = 1
    try:
        app = natron.getInstance(0)
        dot = app.createNode("fr.inria.built-in.Dot")
        assert dot is not None, "createNode(Dot) failed"
        # no frame range and no input: infinite
        frame_range = dot.getFrameRange()
        assert isinstance(frame_range, tuple), frame_range
        assert frame_range == (INT_MIN, INT_MAX), frame_range
        check_layers(dot)
        assert dot.getAvailableLayers(0, 1., 0) == []  # no input
        check_disk_cache(app, dot)
        status = 0
    except BaseException:
        traceback.print_exc()
    natron.quit(status)


main()
