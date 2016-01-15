
OCIO.files = \
$$PWD/OpenColorIO-Configs/ChangeLog \
$$PWD/OpenColorIO-Configs/README \
$$PWD/OpenColorIO-Configs/aces_0.1.1 \
$$PWD/OpenColorIO-Configs/aces_0.7.1 \
$$PWD/OpenColorIO-Configs/blender \
$$PWD/OpenColorIO-Configs/nuke-default \
$$PWD/OpenColorIO-Configs/spi-anim \
$$PWD/OpenColorIO-Configs/spi-vfx

# ACES 1.0.1 also has baked luts and python files which we don't want to bundle
OCIO_aces_101.files = \
$$PWD/OpenColorIO-Configs/aces_1.0.1/config.ocio \
$$PWD/OpenColorIO-Configs/aces_1.0.1/luts


macx {
    OCIO.path = Contents/Resources/OpenColorIO-Configs
    QMAKE_BUNDLE_DATA += OCIO
    OCIO_aces_101.path = Contents/Resources/OpenColorIO-Configs/aces_1.0.1
    QMAKE_BUNDLE_DATA += OCIO_aces_101
}
!macx {
    OCIO.path = $$OUT_PWD/OpenColorIO-Configs
    INSTALLS += OCIO
    OCIO_aces_101.path = $$OUT_PWD/OpenColorIO-Configs/aces_1.0.1
    INSTALLS += OCIO_aces_101
}
