
#include "ofxhBinary.h"
#include "ofxhPluginCache.h"

int
main(int argc, const char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plugin>\n";
        return 1;
    }

    const char* const binaryPath = argv[1];

    // TODO: Change code to _dlHandle = dlopen(_binaryPath.c_str(), RTLD_NOW|RTLD_LOCAL);
    OFX::Binary bin(binaryPath);

    if (bin.isInvalid()) {
        std::cerr << "error: '" << binaryPath << "' is invalid.\n";
        return 1;
    }

    bin.load();

    if (!bin.isLoaded()) {
        std::cerr << "error: '" << binaryPath << "' failed to load.\n";
        return 1;
    }

    auto* getNumberOfPlugins = (OFX::Host::OfxGetNumberOfPluginsFunc)bin.findSymbol("OfxGetNumberOfPlugins");
    auto* getPluginFunc = (OFX::Host::OfxGetPluginFunc)bin.findSymbol("OfxGetPlugin");

    if (getNumberOfPlugins == nullptr || getPluginFunc == nullptr) {
        std::cerr << "error: '" << binaryPath << "' missing required symbols.\n";
        return 1;
    }

    const int numPlugins = getNumberOfPlugins();

    std::cout << "numPlugins: " << numPlugins << "\n";

    if (numPlugins <= 0) {
        std::cerr << "error: unexpected number of plugins.\n";
        return 1;
    }

    for (int i = 0; i < numPlugins; ++i) {
        auto* plugin = getPluginFunc(i);
        if (plugin == nullptr) {
            std::cerr << "Failed to get plugin " << i << "\n";
            return 1;
        }

        std::cout << "plugin[" << i << "] : " << plugin->pluginIdentifier << " v" << plugin->pluginVersionMajor << "." << plugin->pluginVersionMinor << "\n";
    }

    return 0;
}