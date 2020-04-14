const MiniCssExtractPlugin = require("mini-css-extract-plugin")


module.exports = {
    entry: {
        natron: "./stylesheet/main.sass",
    },
    output: {
        path: __dirname + "/",
        filename: "temp/[name].js"
    },
    plugins: [
        new MiniCssExtractPlugin({
            filename: "Gui/Resources/Stylesheets/mainstyle.qss"
        }),
    ],
    resolveLoader: {
        modules: ['node_modules', __dirname + '/loaders']
    },
    module: {
        rules: [
            {
                test: /main.sass/,
                use: [
                    { loader: MiniCssExtractPlugin.loader },
                    {
                        loader: 'css-loader',
                        options: {
                            url: false
                        }
                    },
                    "sass-loader"
                ]
            }
        ]
    }
}