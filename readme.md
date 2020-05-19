# :art: Ppx-Figma

Ppx-Figma will autogenerate styles based on [Bs-Css](https://github.com/reasonml-labs/bs-css) from a figma document for you.

## :thinking: Why ?

The process between developers and designers can be painful. Even if you work carefully and define your styles as variables, you still need to make sure you have the last version of the colors or fonts.
To simplify this process, I made a ppx that automatically import colors and fonts from Figma and create styles based on [Bs-Css](https://github.com/reasonml-labs/bs-css) for you.

## :warning: Important informations

Before you dive in you need to be aware of some points:

-   Ppx-Figma is based on Figma api: - It will require an api key to make request (refer to the Setup part to know how to generate a key), and a request is made on every build - A request is made at each build, so the bigger your Figma document is, the longer the request will be and the longer the build will be. it sounds pretty scary but don't worry there's a cache system and you have control on it
-   By using Ppx-Figma your designer will have to respect an architecture for his fonts and colors. This architecture has been thought with a designer but everyone has his habits. But if you see something to improve feel free to open an issue

## :building_construction: Figma architecture

### Styleguide

All the colors and fonts will have to be in a `Styleguide` page.
So create a new document in Figma, and create a new page called `Styleguide` on the top left:

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/styleguide_before_create.png?raw=true"/>

<br />

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/styleguide_create.png?raw=true"/>

### Fonts

All the fonts will need to be in a frame call `Fonts`. So create the frame with the tool on the top left and call the frame `Fonts` :

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/frame_tool.png?raw=true"/>

<br/>

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/fonts_frame.png?raw=true"/>

Then to define your font styles, create a text, apply your style to it and there you go:

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/fonts_conventions.png?raw=true"/>

:warning: **Important informations**

-   You don't have to "namespace" your styles, but it's a good way to classify them and it will have an impact on generated code
-   The text you write has an importance. For example: `body/regular` will have generate a variable called `regular` in the code. So if you write something like `.../whatever`, a variable called `whatever` will be generated
-   **Only write text for your styles** and don't put annotation next to the styles (I'm planning to allow this is a next version)

### Colors

All the colors will need to be in a frame call `Colors`. So create the frame with the tool on the top left and call the frame `Colors` :

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/frame_tool.png?raw=true"/>

<br/>

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/colors_frame.png?raw=true"/>

Then you can create a rectangle or an ellipse with the desired color:

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/colors_conventions.png?raw=true"/>

Finally name you rectangles or ellipses (here it's the name of the color, but it can be whatever you want of course):

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/colors_namings.png?raw=true"/>

:warning: **Important informations**

-   There's no "namespace" like the text, but if you think this is a needed feature open an issue to notify me
-   Just like the fonts, **only put rectangles or ellipses** and don't put annotation next to them (I'm planning to allow this is a next version)

:tada: That's all for the Figma part :tada:

## :wrench: Setup

### Bs-Css setup

Make sure you have [Bs-Css](https://github.com/reasonml-labs/bs-css) installed. **_If you don't_**, refer to [their documentation](https://github.com/reasonml-labs/bs-css).

### Installation

In a terminal run:

```bash
npm i ppx-figma
OR
yarn add ppx-figma
```

Then add the dependency to the `bs-config.json`

```json
...
"ppx-flags": ["ppx-figma/ppx"]
...
```

### Get a Figma token

You'll need to generate a token to allow the request on the document. Here are the steps to generate one:

-   Go on [Figma](https://www.figma.com/)
-   Click on your profile on the top left

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/token_profile.png?raw=true"/>

-   Then go in the settings

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/token_settings.png?raw=true"/>

-   Finally generate a token

<img src="https://github.com/JulesGuesnon/Ppx-Figma/blob/master/screenshots/token_generate.png?raw=true"/>

### Get a document id

The last thing to do is to get the document id. It's pretty simple, you'll find it in the url of the document:
`https://www.figma.com/file/the_id_is_this_part_of_the_url`

## :fire: How to use it ?

Now we're ready to go !

Here is the syntax of the ppx:

```reason
open Css;

%figma
["your token", "the file id", "cache time"]
```

:construction: **STILL WIP** :construction:
