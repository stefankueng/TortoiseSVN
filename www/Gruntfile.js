'use strict';

var autoprefixer = require('autoprefixer');

module.exports = function(grunt) {
    // Load any grunt plugins found in package.json.
    require('load-grunt-tasks')(grunt, { scope: 'devDependencies' });
    require('time-grunt')(grunt);

    grunt.initConfig({
        dirs: {
            dest: 'dist',
            src: 'source'
        },

        jekyll: {
            site: {
                options: {
                    bundleExec: true,
                    incremental: false
                }
            }
        },

        htmlmin: {
            dist: {
                options: {
                    collapseBooleanAttributes: true,
                    collapseInlineTagWhitespace: false,
                    collapseWhitespace: true,
                    conservativeCollapse: false,
                    decodeEntities: true,
                    ignoreCustomComments: [/^\s*google(off|on):\s/],
                    minifyCSS: {
                        level: {
                            1: {
                                specialComments: 0
                            }
                        }
                    },
                    minifyJS: true,
                    minifyURLs: false,
                    processConditionalComments: true,
                    removeAttributeQuotes: true,
                    removeComments: true,
                    removeOptionalAttributes: true,
                    removeOptionalTags: true,
                    removeRedundantAttributes: true,
                    removeScriptTypeAttributes: true,
                    removeStyleLinkTypeAttributes: true,
                    removeTagWhitespace: false,
                    sortAttributes: true,
                    sortClassName: true
                },
                expand: true,
                cwd: '<%= dirs.dest %>',
                dest: '<%= dirs.dest %>',
                src: ['**/*.html']
            }
        },

        concat: {
            css: {
                src: ['<%= dirs.src %>/assets/css/vendor/normalize.css',
                      '<%= dirs.src %>/assets/css/vendor/baguetteBox.css',
                      '<%= dirs.src %>/assets/css/vendor/highlighter.css',
                      '<%= dirs.src %>/assets/css/flags-sprite.css',
                      '<%= dirs.src %>/assets/css/style.css'
                ],
                dest: '<%= dirs.dest %>/assets/css/pack.css'
            },
            baguetteBox: {
                src: ['<%= dirs.src %>/assets/js/vendor/baguetteBox.js',
                      '<%= dirs.src %>/assets/js/baguetteBox-init.js'
                ],
                dest: '<%= dirs.dest %>/assets/js/baguetteBox-pack.js'
            }
        },

        postcss: {
            options: {
                processors: [
                    autoprefixer() // add vendor prefixes
                ]
            },
            dist: {
                src: '<%= concat.css.dest %>'
            }
        },

        purgecss: {
            dist: {
                options: {
                    content: [
                        '<%= dirs.dest %>/**/*.html',
                        '<%= dirs.dest %>/assets/js/**/*.js'
                    ],
                    keyframes: true,
                    safelist: [
                        /bounce-from-/
                    ]
                },
                files: {
                    '<%= concat.css.dest %>': ['<%= concat.css.dest %>']
                }
            }
        },

        cssmin: {
            minify: {
                options: {
                    level: {
                        1: {
                            specialComments: 0
                        }
                    }
                },
                files: {
                    '<%= concat.css.dest %>': '<%= concat.css.dest %>'
                }
            }
        },

        uglify: {
            options: {
                compress: true,
                mangle: true,
                output: {
                    comments: false
                }
            },
            minify: {
                files: {
                    '<%= concat.baguetteBox.dest %>': '<%= concat.baguetteBox.dest %>'
                }
            }
        },

        filerev: {
            css: {
                src: '<%= dirs.dest %>/assets/css/**/*.css'
            },
            js: {
                src: [
                    '<%= dirs.dest %>/assets/js/**/*.js'
                ]
            },
            images: {
                src: [
                    '<%= dirs.dest %>/assets/img/**/*.{jpg,jpeg,gif,png,svg}',
                    '!<%= dirs.dest %>/assets/img/logo-256x256.png'
                ]
            }
        },

        useminPrepare: {
            html: '<%= dirs.dest %>/index.html',
            options: {
                dest: '<%= dirs.dest %>',
                root: '<%= dirs.dest %>'
            }
        },

        usemin: {
            css: '<%= dirs.dest %>/assets/css/pack*.css',
            html: '<%= dirs.dest %>/**/*.html',
            options: {
                assetsDirs: ['<%= dirs.dest %>/', '<%= dirs.dest %>/assets/img/']
            }
        },

        svgmin: {
            options: {
                multipass: true,
                plugins: [
                    { cleanupAttrs: true },
                    { cleanupEnableBackground: true },
                    { cleanupIDs: true },
                    { cleanupListOfValues: true },
                    { cleanupNumericValues: true },
                    { collapseGroups: true },
                    { convertColors: true },
                    { convertPathData: true },
                    { convertShapeToPath: true },
                    { convertStyleToAttrs: true },
                    { convertTransform: true },
                    { inlineStyles: true },
                    { mergePaths: true },
                    { minifyStyles: true },
                    { moveElemsAttrsToGroup: true },
                    { moveGroupAttrsToElems: true },
                    {
                        removeAttrs: {
                            attrs: 'data-name'
                        }
                    },
                    { removeComments: true },
                    { removeDesc: true },
                    { removeDoctype: true },
                    { removeEditorsNSData: true },
                    { removeEmptyAttrs: true },
                    { removeEmptyContainers: true },
                    { removeEmptyText: true },
                    { removeHiddenElems: true },
                    { removeMetadata: true },
                    { removeNonInheritableGroupAttrs: true },
                    { removeTitle: true },
                    {
                        removeUnknownsAndDefaults: {
                            keepRoleAttr: true
                        }
                    },
                    { removeUnusedNS: true },
                    { removeUselessDefs: true },
                    { removeUselessStrokeAndFill: true },
                    { removeViewBox: false },
                    { removeXMLNS: false },
                    { removeXMLProcInst: true },
                    { sortAttrs: true }
                ]
            },
            dist: {
                expand: true,
                cwd: '<%= dirs.dest %>',
                dest: '<%= dirs.dest %>',
                src: 'assets/img/**/*.svg'
            }
        },

        connect: {
            options: {
                base: '<%= dirs.dest %>/',
                hostname: 'localhost'
            },
            livereload: {
                options: {
                    base: '<%= dirs.dest %>/',
                    livereload: 35729,
                    // Automatically open the webpage in the default browser
                    open: true,
                    port: 8000
                }
            },
            linkChecker: {
                options: {
                    base: '<%= dirs.dest %>/',
                    port: 9001
                }
            }
        },

        watch: {
            options: {
                livereload: '<%= connect.livereload.options.livereload %>'
            },
            dev: {
                files: [
                    '<%= dirs.src %>/**',
                    '.jshintrc',
                    '_config.yml',
                    'Gruntfile.js'
                ],
                tasks: 'dev'
            },
            build: {
                files: [
                    '<%= dirs.src %>/**',
                    '.jshintrc',
                    '_config.yml',
                    'Gruntfile.js'
                ],
                tasks: 'build'
            }
        },

        csslint: {
            options: {
                csslintrc: '.csslintrc'
            },
            src: '<%= dirs.src %>/assets/css/style.css'
        },

        jshint: {
            options: {
                jshintrc: '.jshintrc'
            },
            files: {
                src: [
                    'Gruntfile.js',
                    '<%= dirs.src %>/assets/js/*.js'
                ]
            }
        },

        htmllint: {
            src: '<%= dirs.dest %>/**/*.html'
        },

        linkChecker: {
            options: {
                callback: function (crawler) {
                    crawler.addFetchCondition(function (queueItem) {
                        return !queueItem.path.match(/\/docs\/(release|nightly)\//);
                    });
                },
                interval: 1, // 1 ms; default 250
                maxConcurrency: 5 // default; bigger doesn't seem to improve time
            },
            dev: {
                site: 'localhost',
                options: {
                    initialPort: '<%= connect.linkChecker.options.port %>'
                }
            }
        },

        clean: {
            dist: '<%= dirs.dest %>/'
        }

    });

    grunt.registerTask('build', [
        'clean',
        'jekyll',
        'useminPrepare',
        'concat',
        'postcss',
        'purgecss',
        'cssmin',
        'uglify',
        'filerev',
        'usemin',
        'svgmin',
        'htmlmin'
    ]);

    grunt.registerTask('test', [
        'build',
        'csslint',
        'jshint',
        'htmllint',
        'connect:linkChecker',
        'linkChecker'
    ]);

    grunt.registerTask('dev', [
        'jekyll',
        'useminPrepare',
        'concat',
        'postcss',
        'filerev',
        'usemin'
    ]);

    grunt.registerTask('server', [
        'build',
        'connect:livereload',
        'watch:build'
    ]);

    grunt.registerTask('default', [
        'dev',
        'connect:livereload',
        'watch:dev'
    ]);
};
