open Ppxlib;

module Api = Figma_Api;

module Types = {
  type color_ = {
    r: float,
    g: float,
    b: float,
    a: float,
  };

  type paintType =
    | SOLID
    | GRADIENT_LINEAR
    | GRADIENT_RADIAL
    | GRADIENT_ANGULAR
    | GRADIENT_DIAMOND
    | IMAGE
    | EMOJI;

  type paint = {
    type_: paintType,
    visible: option(bool),
    opacity: float,
    color: color_,
  };

  type absoluteBoundingBox = {
    x: float,
    y: float,
    width: int,
    height: int,
  };

  type strokeType =
    | SOLID;

  type paints = list(paint);

  type strokeAlign =
    | INSIDE
    | OUTSIDE
    | CENTER;

  type primitive =
    | String(string)
    | Int(int)
    | Float(float)
    | Null;

  type text = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    styles: list((string, primitive)),
    characters: string,
  };

  type size = {
    x: float,
    y: float,
  };

  type effect = {
    type_: string,
    visible: bool,
    color: color_,
    offset: size,
    radius: int,
  };

  type vector = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    fills: paints,
    strokes: paints,
    strokeAlign,
    opacity: option(float),
    size: option(size),
    styles: option(list((string, primitive))),
  };

  type rectangle = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    fills: paints,
    strokes: paints,
    strokeAlign,
    opacity: option(float),
    size: option(size),
    styles: option(list((string, primitive))),
    cornerRadius: float,
    rectangleCornerRadii: list(float),
  };

  type frameChild =
    | Rectangle(rectangle)
    | Vector(vector)
    | Text(text)
    | Group(frame)
    | Component(component)
  and frame = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    children: list(frameChild),
    locked: option(bool),
    backgroundColor: color_,
    strokeWeight: float,
    strokeAlign,
  }
  and component = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    children: list(frameChild),
    backgroundColor: color_,
    strokeWeight: float,
    strokes: paints,
    strokeAlign,
    fills: paints,
    rectangleCornerRadii: list(float),
    absoluteBoundingBox,
    effects: list(effect),
  };

  type canvasChild =
    | Component(component)
    | Frame(frame);

  type canvas = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    children: list(canvasChild),
    backgroundColor: color_,
  };

  type document = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    children: list(canvas),
  };

  type nodeType =
    | NodeText
    | NodeVector
    | NodeRectangle
    | NodeGroup
    | NodeDocument
    | NodeCanvas
    | NodeComponent
    | NodeFrame;
};
open Types;

module Parser = {
  open Yojson.Basic.Util;

  exception PaintExn(string);
  exception StrokeAlignExn(string);
  exception NodeTypeExn(string);
  exception StyleValueExn(string);
  exception CanvasChildExn(string);

  let invalidCssKeys = [
    "fontPostScriptName",
    "textAlignHorizontal",
    "textAlignVertical",
    "lineHeightPercent",
    "lineHeightUnit",
    "lineHeightPercentFontSize",
    "textAutoResize",
    "textAlignHorizontal",
    "textAlignVertical",
  ];

  let toRewriteCssKeys = [("lineHeightPx", "lineHeight")];

  let parseColor_ = color => {
    r: color |> member("r") |> to_float,
    g: color |> member("g") |> to_float,
    b: color |> member("b") |> to_float,
    a: color |> member("a") |> to_float,
  };

  let parsePaintType: Yojson.Basic.t => paintType =
    paintType =>
      switch (to_string(paintType)) {
      | "SOLID" => SOLID
      | "GRADIENT_LINEAR" => GRADIENT_LINEAR
      | "GRADIENT_RADIAL" => GRADIENT_RADIAL
      | "GRADIENT_ANGULAR" => GRADIENT_ANGULAR
      | "GRADIENT_DIAMOND" => GRADIENT_DIAMOND
      | "IMAGE" => IMAGE
      | "EMOJI" => EMOJI
      | str =>
        raise(
          PaintExn(
            "The following paintType: " ++ str ++ " isn't a valid one",
          ),
        )
      };

  let parseAbsoluteBoundingBox = absoluteBoundingBox => {
    x: absoluteBoundingBox |> member("x") |> to_float,
    y: absoluteBoundingBox |> member("y") |> to_float,
    width: absoluteBoundingBox |> member("width") |> to_int,
    height: absoluteBoundingBox |> member("height") |> to_int,
  };

  let parsePaint = paint => {
    {
      type_: paint |> member("type") |> parsePaintType,
      visible: paint |> member("visible") |> to_bool_option,
      opacity:
        paint
        |> member("opacity")
        |> to_float_option
        |> Utils.Options.Float.valueOrDefault(1.0),
      color: paint |> member("color") |> parseColor_,
    };
  };

  let parseStrokeAlign = strokeAlign => {
    switch (to_string(strokeAlign)) {
    | "INSIDE" => INSIDE
    | "OUTSIDE" => OUTSIDE
    | "CENTER" => CENTER
    | value =>
      raise(
        StrokeAlignExn(
          "The following value is does not belongs to type stokeAlign: "
          ++ value,
        ),
      )
    };
  };

  let getNodeType = node => {
    let type_ = node |> member("type") |> to_string;
    switch (type_) {
    | "DOCUMENT" => NodeDocument
    | "CANVAS" => NodeCanvas
    | "FRAME" => NodeFrame
    | "TEXT" => NodeText
    | "RECTANGLE" => NodeRectangle
    | "GROUP" => NodeGroup
    | "STAR"
    | "LINE"
    | "REGULAR_POLYGON"
    | "ELLIPSE" => NodeVector
    | "COMPONENT" => NodeComponent
    | n =>
      raise(
        NodeTypeExn(
          Printf.sprintf("The node type '%s' couldn't be found", n),
        ),
      )
    };
  };

  let parseSize = node => {
    x: node |> member("x") |> to_float,
    y: node |> member("y") |> to_float,
  };

  let parseStyles = node => {
    let stylesKey = node |> keys;
    let stylesValues = node |> values;
    let styles = ref([]);

    stylesKey
    |> List.iteri((i, key) => {
         let currentValue = List.nth(stylesValues, i);
         let value =
           switch (currentValue) {
           | `String(s) => String(s)
           | `Int(i) => Int(i)
           | `Float(f) => Float(f)
           | `Null => Null
           | _ =>
             raise(
               StyleValueExn(
                 "The type wasn't supported while parsing styles, e.g: not a string, int, float or null",
               ),
             )
           };

         styles := styles^ |> List.append([(key, value)]);
       });

    styles^
    |> List.filter(((key, _)) =>
         !List.exists(invalidKey => key == invalidKey, invalidCssKeys)
       )
    |> List.map(((key, value)) => {
         let (_, key) =
           try(List.find(((oldKey, _)) => oldKey == key, toRewriteCssKeys)) {
           | _ => ("", key)
           };

         (key, value);
       });
  };

  let parseEffect = effect => {
    type_: effect |> member("type") |> to_string,
    visible: effect |> member("visible") |> to_bool,
    color: effect |> member("color") |> parseColor_,
    offset: effect |> member("offset") |> parseSize,
    radius: effect |> member("radius") |> to_int,
  };

  let parseVector = node => {
    {
      id: node |> member("id") |> to_string,
      name: node |> member("name") |> to_string,
      visible: node |> member("visible") |> to_bool_option,
      type_: node |> member("type") |> to_string,
      fills: node |> member("fills") |> to_list |> List.map(parsePaint),
      strokes: node |> member("strokes") |> to_list |> List.map(parsePaint),
      strokeAlign: node |> member("strokeAlign") |> parseStrokeAlign,
      opacity: node |> member("opacity") |> to_float_option,
      size:
        switch (node |> member("size")) {
        | `Null => None
        | v => Some(parseSize(v))
        },
      styles:
        switch (node |> member("style")) {
        | `Null => None
        | v => Some(parseStyles(v))
        },
    };
  };

  let parseRectangle = node => {
    {
      id: node |> member("id") |> to_string,
      name: node |> member("name") |> to_string,
      visible: node |> member("visible") |> to_bool_option,
      type_: node |> member("type") |> to_string,
      fills: node |> member("fills") |> to_list |> List.map(parsePaint),
      strokes: node |> member("strokes") |> to_list |> List.map(parsePaint),
      strokeAlign: node |> member("strokeAlign") |> parseStrokeAlign,
      opacity: node |> member("opacity") |> to_float_option,
      size:
        switch (node |> member("size")) {
        | `Null => None
        | v => Some(parseSize(v))
        },
      styles:
        switch (node |> member("style")) {
        | `Null => None
        | v => Some(parseStyles(v))
        },
      cornerRadius:
        node
        |> member("cornerRadius")
        |> to_float_option
        |> Utils.Options.Float.valueOrDefault(0.),
      rectangleCornerRadii:
        switch (node |> member("rectangleCornerRadii") |> to_option(to_list)) {
        | Some(v) => v |> List.map(to_float)
        | None => []
        },
    };
  };

  let parseText = textNode => {
    {
      id: textNode |> member("id") |> to_string,
      name: textNode |> member("name") |> to_string,
      visible: textNode |> member("visible") |> to_bool_option,
      type_: textNode |> member("type") |> to_string,
      characters: textNode |> member("characters") |> to_string,
      styles: textNode |> member("style") |> parseStyles,
    };
  };

  let rec parseFrame = frameNode => {
    let children =
      frameNode
      |> member("children")
      |> to_list
      |> List.map(node => {
           let nodeType = getNodeType(node);
           switch (nodeType) {
           | NodeText => Text(parseText(node))
           | NodeRectangle => Rectangle(parseRectangle(node))
           | NodeVector => Vector(parseVector(node))
           | NodeGroup => Group(parseFrame(node))
           | NodeComponent => Component(parseComponent(node))
           | _ => raise(NodeTypeExn("Node type not supported"))
           };
         });

    {
      id: frameNode |> member("id") |> to_string,
      name: frameNode |> member("name") |> to_string,
      visible: frameNode |> member("visible") |> to_bool_option,
      type_: frameNode |> member("type") |> to_string,
      children,
      locked: frameNode |> member("locked") |> to_bool_option,
      backgroundColor: frameNode |> member("backgroundColor") |> parseColor_,
      strokeWeight: frameNode |> member("strokeWeight") |> to_float,
      strokeAlign: frameNode |> member("strokeAlign") |> parseStrokeAlign,
    };
  }

  and parseComponent = componentNode => {
    let children =
      componentNode
      |> member("children")
      |> to_list
      |> List.map(node => {
           let nodeType = getNodeType(node);
           switch (nodeType) {
           | NodeText => Text(parseText(node))
           | NodeVector => Vector(parseVector(node))
           | NodeRectangle => Rectangle(parseRectangle(node))
           | NodeGroup => Group(parseFrame(node))
           | _ => raise(NodeTypeExn("Node type not supported"))
           };
         });

    {
      id: componentNode |> member("id") |> to_string,
      name: componentNode |> member("name") |> to_string,
      visible: componentNode |> member("visible") |> to_bool_option,
      type_: componentNode |> member("type") |> to_string,
      children,
      backgroundColor:
        componentNode |> member("backgroundColor") |> parseColor_,
      strokeWeight: componentNode |> member("strokeWeight") |> to_float,
      strokes:
        componentNode |> member("strokes") |> to_list |> List.map(parsePaint),
      strokeAlign: componentNode |> member("strokeAlign") |> parseStrokeAlign,
      fills:
        componentNode |> member("fills") |> to_list |> List.map(parsePaint),
      rectangleCornerRadii:
        switch (
          componentNode
          |> member("rectangleCornerRadii")
          |> to_option(to_list)
        ) {
        | Some(v) => v |> List.map(to_float)
        | None => []
        },

      absoluteBoundingBox:
        componentNode
        |> member("parseAbsoluteBoundingBox")
        |> parseAbsoluteBoundingBox,
      effects:
        componentNode
        |> member("effects")
        |> to_list
        |> List.map(parseEffect),
    };
  };

  let parseCanvasChild = canvasChild => {
    switch (canvasChild |> member("type") |> to_string) {
    | "COMPONENT" => Component(parseComponent(canvasChild))
    | "FRAME" => Frame(parseFrame(canvasChild))
    | str =>
      raise(
        CanvasChildExn(
          "The type: " ++ str ++ "isn't supported as a canvasChild",
        ),
      )
    };
  };

  let parseCanvas = canvasNode => {
    {
      id: canvasNode |> member("id") |> to_string,
      name: canvasNode |> member("name") |> to_string,
      visible: canvasNode |> member("visible") |> to_bool_option,
      type_: canvasNode |> member("type") |> to_string,
      backgroundColor: canvasNode |> member("backgroundColor") |> parseColor_,
      children:
        canvasNode
        |> member("children")
        |> to_list
        |> List.map(node => {
             switch (getNodeType(node)) {
             | NodeComponent => Component(parseComponent(node))
             | NodeFrame => Frame(parseFrame(node))
             | _ =>
               raise(
                 NodeTypeExn(
                   "Canvas child node wasn't of type Component or Frame",
                 ),
               )
             }
           }),
    };
  };

  let parseDocument = root => {
    let docNode = member("document", root);

    {
      id: docNode |> member("id") |> to_string,
      name: docNode |> member("name") |> to_string,
      visible: docNode |> member("visible") |> to_bool_option,
      type_: docNode |> member("type") |> to_string,
      children:
        docNode
        |> member("children")
        |> to_list
        |> List.filter(canvasNode =>
             canvasNode |> member("name") |> to_string == "Styleguide"
           )
        |> List.map(parseCanvas),
    };
  };

  let parse = str => Yojson.Basic.from_string(str) |> parseDocument;
};

let getCanvasByName = (~name, document: document) =>
  switch (
    document.children |> List.find((canvas: canvas) => canvas.name == name)
  ) {
  | v => Some(v)
  | exception _ => None
  };

let getFrameByName = (~name, canvas: canvas) =>
  switch (
    canvas.children
    |> List.find(frameChild => {
         switch (frameChild) {
         | Frame(frame) => frame.name == name
         | _ => false
         }
       })
  ) {
  | Frame(v) => Some(v)
  | _ => None
  | exception _ => None
  };

let getTexts = document => {
  switch (getCanvasByName(~name="Styleguide", document)) {
  | Some(c) =>
    switch (getFrameByName(~name="Fonts", c)) {
    | Some(f) =>
      Some(
        f.children
        |> List.map(child => {
             switch (child) {
             | Text(t) => t
             | _ =>
               raise(
                 Parser.NodeTypeExn(
                   "Error while getting texts: a node wasn't of type text",
                 ),
               )
             }
           }),
      )
    | None => None
    }
  | None => None
  };
};

let getColors = document => {
  switch (getCanvasByName(~name="Styleguide", document)) {
  | Some(c) =>
    switch (getFrameByName(~name="Colors", c)) {
    | Some(f) =>
      Some(
        f.children
        |> List.map(child => {
             switch (child) {
             | Rectangle(r) => (r.name, r.fills)
             | Vector(v) => (v.name, v.fills)
             | _ =>
               raise(
                 Parser.NodeTypeExn(
                   "Error while getting colors: a node wasn't of type vector",
                 ),
               )
             }
           }),
      )
    | None => None
    }
  | None => None
  };
};

exception GetComponentsFilterExn(string);

let extractComponent = (child: frameChild) =>
  switch (child) {
  | Component(c) => c
  | _ =>
    raise(
      GetComponentsFilterExn(
        "An error occured when getting components: the children of the frames are supposed to be filtered and only contains components, but it wasn't the case",
      ),
    )
  };

let rec getComponentsOfFrameChild = children => {
  [
    children
    |> List.filter((child: frameChild) =>
         switch (child) {
         | Component(_) => true
         | _ => false
         }
       )
    |> List.map(extractComponent),
    children
    |> List.filter((child: frameChild) =>
         switch (child) {
         | Component(_) => false
         | _ => true
         }
       )
    |> getComponentsOfFrameChild,
  ]
  |> List.flatten;
};

let getComponents = document => {
  [];
    /* document.children
       |> List.map((canvas: canvas) => {
            canvas.children
            |> List.map((child: canvasChild) =>
                 switch (child) {
                 | Component(c) => [c]
                 | Frame(f) => f.children |> getComponentsOfFrameChild
                 }
               )
            |> List.flatten
          })
       |> List.flatten; */
};

let unitOfKeyAndValue = (cssProps, value) =>
  switch (cssProps, value) {
  | ("fontFamily", String(s)) => (
      (~loc) =>
        Utils.Ast.makePolyVariant(
          ~label="custom",
          ~loc,
          Some({
            pexp_desc: Pexp_constant(Pconst_string(s, None)),
            pexp_loc: loc,
            pexp_loc_stack: [],
            pexp_attributes: [],
          }),
        )
    )
  | ("fontWeight", Int(i)) => (
      (~loc) =>
        Utils.Ast.makePolyVariant(
          ~loc,
          ~label="num",
          Some({
            pexp_desc:
              Pexp_constant(Pconst_integer(string_of_int(i), None)),
            pexp_loc: loc,
            pexp_loc_stack: [],
            pexp_attributes: [],
          }),
        )
    )
  | ("fontWeight", Float(f)) => (
      (~loc) =>
        Utils.Ast.makePolyVariant(
          ~loc,
          ~label="num",
          Some({
            pexp_desc:
              Pexp_constant(Pconst_float(string_of_float(f), None)),
            pexp_loc: loc,
            pexp_loc_stack: [],
            pexp_attributes: [],
          }),
        )
    )
  | (_, Int(i)) => (
      (~loc) =>
        Utils.Ast.makeFunction(
          ~loc,
          ~label="px",
          [(Nolabel, Utils.Ast.makeInt(~loc, i))],
        )
    )
  | (_, Float(f)) => (
      (~loc) =>
        Utils.Ast.makeFunction(
          ~loc,
          ~label="pxFloat",
          [(Nolabel, Utils.Ast.makeFloat(~loc, f))],
        )
    )
  | (_, String(s)) => ((~loc) => Utils.Ast.makeString(~loc, s))
  | _ => ((~loc) => Utils.Ast.makeString(~loc, ""))
  };