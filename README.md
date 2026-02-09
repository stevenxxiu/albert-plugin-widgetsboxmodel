# Albert plugin: Widgets BoxModel

The official frontend for Albert.

---
---

**NOTE**: The styling feature is in development. Expect things to break. ⚠️🚧

However, if you are keen on styling, feedback is highly appreciated. 
Join the communtiy chats and discuss.

---
---


## Styleing (v0.0)

A style is an INI-formated plain-text file containing grouped key–value pairs.
Values of other keys can be referenced by the name of the key preceeded by a dollar sign '$'.


The `palette` group is used to derive colors for general widget drawing.
All keys in this group represent colors.
The `window` group is used to explicitly override window colors.
Depending on the key names in this group, values are expected to be colors or brushes.
The unnamed root group can be used to define variables that you may want to reference later.

Besides the shipped styles, you can find a style template file in the system data directory.
It contains the supported keys for each group, along with a description.


### Color

Colors may be in one of these formats:

- #RGB (each of R, G, and B is a single hex digit)
- #RRGGBB
- #AARRGGBB (AA is the alpha channel)
- #RRRGGGBBB
- #RRRRGGGGBBBB
- A name from the list of colors defined in the list of SVG color keyword names.
- transparent - representing the absence of a color.


### Brushes 

Brushes can be either a color, as defined above, or a brush function (see below).


#### Brush functions

All brush functions use the following syntax:

```
function_name(<designated arguments>)
```

Designated arguments are a comma-separated list of parameter–argument pairs of the form:

```
parameter: arguments...
```

Multiple occurrences of the same parameter are allowed, 
and the order of parameters does not matter.
Each provided argument may be a list of space-separated values.
The order of these space-separated values does matter.


##### Linear gradients

```
linear-gradient(x1:<double>, y1:<double>,    # Gradient start point
                x2:<double>, y2:<double>,    # Gradient end point
                stop: <double> <color>, ...) # Gradient stop(s)        
```

See https://doc.qt.io/qt-6/qlineargradient.html


##### Images

```
image(src:<path>)  # Gradient start point
```

- ***src***:
  Absolute path or relative path to the style file. 

### Useful tools 

- https://paletton.com/
- https://www.fffuel.co/
- https://pattern.monster/
- https://webgradients.com/
- https://bgjar.com/
- https://www.svgbackgrounds.com/set/free-svg-backgrounds-and-patterns/
- Feel free to add resources here!

