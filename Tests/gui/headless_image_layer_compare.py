"""ImageLayer == and < compare the layers, not the Python objects (#1810).

Run by NatronRenderer -t (see Tests/CMakeLists.txt).
"""
import NatronEngine
from NatronEngine import natron

ImageLayer = NatronEngine.ImageLayer

status = 1
try:
    rgba = ImageLayer.getRGBAComponents()
    assert rgba == ImageLayer.getRGBAComponents()
    assert not rgba != ImageLayer.getRGBAComponents()
    assert rgba != ImageLayer.getRGBComponents()
    assert rgba != ImageLayer.getAlphaComponents()
    # same channels, another plane
    assert ImageLayer.getForwardMotionComponents() != ImageLayer.getBackwardMotionComponents()

    a = ImageLayer("MyLayer", "XY", ["X", "Y"])
    assert a == ImageLayer("MyLayer", "XY", ["X", "Y"])
    assert a != ImageLayer("MyLayer", "XYZ", ["X", "Y", "Z"])
    assert a != ImageLayer("MyLayer", "UV", ["U", "V"])
    assert a != ImageLayer("Other", "XY", ["X", "Y"])

    # < is a strict weak ordering consistent with ==
    layers = [
        ImageLayer.getRGBAComponents(), ImageLayer.getRGBComponents(), ImageLayer.getAlphaComponents(),
        ImageLayer.getForwardMotionComponents(), ImageLayer.getBackwardMotionComponents(),
        ImageLayer.getDisparityLeftComponents(), ImageLayer.getDisparityRightComponents(), a,
        ImageLayer("MyLayer", "UV", ["U", "V"]),
    ]
    for x in layers:
        assert not x < x
        for y in layers:
            assert (x == y) == (not x < y and not y < x), (x.getLayerName(), y.getLayerName())

    # equal layers have the same hash, so they can be dict keys and set items
    assert hash(rgba) == hash(ImageLayer.getRGBAComponents())
    assert hash(a) == hash(ImageLayer("MyLayer", "XY", ["X", "Y"]))
    # the channels label is only a display name
    assert a == ImageLayer("MyLayer", "Position", ["X", "Y"])
    assert hash(a) == hash(ImageLayer("MyLayer", "Position", ["X", "Y"]))
    assert len(set(layers + [ImageLayer.getRGBAComponents(), ImageLayer("MyLayer", "XY", ["X", "Y"])])) == len(layers)
    assert {rgba: 1}.get(ImageLayer.getRGBAComponents()) == 1
    assert ImageLayer.getRGBAComponents() in layers
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
