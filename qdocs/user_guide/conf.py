# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'Qualcomm Telematics SDK User Guide'
copyright = 'Qualcomm Innovation Center, Inc.'
author = 'Qualcomm Innovation Center, Inc.'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx_rtd_theme'
#   'sphinx.ext.autosectionlabel'
]

autosectionlabel_prefix_document = True
autosectionlabel_maxdepth = 4

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.

# _ignore, generate_qs_project, and the venvs are nonstandard exludes set up for this project specifically. _ignore is included as an example and is not used in this project. 

exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'ug-source/api_samples/README.md']

source_suffix = {
    '.rst': 'restructuredtext'
}



# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'
html_static_path = ['../_static']

html_copy_source = False
html_show_sourcelink = False
html_show_sphinx = False

html_title = ''
html_css_files = ['custom_css.css']
