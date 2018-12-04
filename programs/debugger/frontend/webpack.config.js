const ExtractTextPlugin = require("extract-text-webpack-plugin");
const webpack = require('webpack');

module.exports = {
   entry: "./src/index.tsx",
   output: {
       filename: "bundle.js",
       path: __dirname + "/dist"
   },

   // Enable sourcemaps for debugging webpack's output.
   devtool: "source-map",

   resolve: {
       // Add '.ts' and '.tsx' as resolvable extensions.
       extensions: [".ts", ".tsx", ".js", ".json"]
   },

   module: {
       rules: [
           // All files with a '.ts' or '.tsx' extension will be handled by 'awesome-typescript-loader'.
           { test: /\.tsx?$/, loader: "awesome-typescript-loader" },
           { test: /\.css$/, 
            use: ExtractTextPlugin.extract({ 
              use: 'css-loader', 
              fallback: 'style-loader' 
            }
            )},

           // All output '.js' files will have any sourcemaps re-processed by 'source-map-loader'.
           { enforce: "pre", test: /\.js$/, loader: "source-map-loader" }
       ]
   },
   plugins: [
      new ExtractTextPlugin("styles.css"),
      new webpack.DefinePlugin({
         'process.env.NODE_ENV': JSON.stringify('production')
      })
   ],
   // When importing a module whose path matches one of the following, just
   // assume a corresponding global variable exists and use that instead.
   // This is important because it allows us to avoid bundling all of our
   // dependencies, which allows browsers to cache those libraries between builds.
   externals: {
   },
};
