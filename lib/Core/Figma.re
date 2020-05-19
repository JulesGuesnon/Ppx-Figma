open Ppxlib;

module Api = Figma_Api;

module Types = {
  type color_ = {
    r: float,
    g: float,
    b: float,
    a: float,
  };

  type paint =
    | SOLID
    | GRADIENT_LINEAR
    | GRADIENT_RADIAL
    | GRADIENT_ANGULAR
    | GRADIENT_DIAMOND
    | IMAGE
    | EMOJI;

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

  type fill = {color: color_};

  type color = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    fills: list(fill),
  };

  type frameChild =
    | Color(color)
    | Text(text);

  type frame = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    children: list(frameChild),
    locked: option(bool),
    backgroundColor: color_,
    strokeWeight: float,
    strokeAlign,
  };

  type canvas = {
    id: string,
    name: string,
    visible: option(bool),
    type_: string,
    children: list(frame),
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
    | NodeColor
    | NodeDocument
    | NodeCanvas;
};
open Types;

module Parser = {
  open Yojson.Basic.Util;

  exception PaintExn(string);
  exception StrokeAlignExn(string);
  exception NodeTypeExn(string);
  exception StyleValueExn(string);

  let invalidCssKeys = [
    "fontPostScriptName",
    "textAlignHorizontal",
    "textAlignVertical",
    "lineHeightPercent",
    "lineHeightUnit",
    "lineHeightPercentFontSize",
  ];

  let toRewriteCssKeys = [("lineHeightPx", "lineHeight")];

  let parseColor_ = color => {
    r: color |> member("r") |> to_float,
    g: color |> member("g") |> to_float,
    b: color |> member("b") |> to_float,
    a: color |> member("a") |> to_float,
  };

  let parsePaint = paint => {
    switch (to_string(paint)) {
    | "SOLID" => SOLID
    | "GRADIENT_LINEAR" => GRADIENT_LINEAR
    | "GRADIENT_RADIAL" => GRADIENT_RADIAL
    | "GRADIENT_ANGULAR" => GRADIENT_ANGULAR
    | "GRADIENT_DIAMOND" => GRADIENT_DIAMOND
    | "IMAGE" => IMAGE
    | "EMOJI" => EMOJI
    | value =>
      raise(
        PaintExn(
          "The following value is does not belongs to type paint: " ++ value,
        ),
      )
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
    switch (node |> member("type") |> to_string) {
    | "DOCUMENT" => NodeDocument
    | "CANVAS" => NodeCanvas
    | "TEXT" => NodeText
    | "ELLIPSE"
    | "RECTANGLE" => NodeColor
    | _ => raise(NodeTypeExn("The node type couldn't be found"))
    };
  };

  let parseFill = fill => {color: fill |> member("color") |> parseColor_};

  let parseColor = node => {
    id: node |> member("id") |> to_string,
    name: node |> member("name") |> to_string,
    visible: node |> member("visible") |> to_bool_option,
    type_: node |> member("type") |> to_string,
    fills: node |> member("fills") |> to_list |> List.map(parseFill),
  };

  let parseText = textNode => {
    let stylesKey = textNode |> member("style") |> keys;
    let stylesValues = textNode |> member("style") |> values;
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

    {
      id: textNode |> member("id") |> to_string,
      name: textNode |> member("name") |> to_string,
      visible: textNode |> member("visible") |> to_bool_option,
      type_: textNode |> member("type") |> to_string,
      characters: textNode |> member("characters") |> to_string,
      styles:
        styles^
        |> List.filter(((key, _)) =>
             !List.exists(invalidKey => key == invalidKey, invalidCssKeys)
           )
        |> List.map(((key, value)) => {
             let (_, key) =
               try(
                 List.find(((oldKey, _)) => oldKey == key, toRewriteCssKeys)
               ) {
               | _ => ("", key)
               };

             (key, value);
           }),
    };
  };

  let parseFrame = frameNode => {
    let children =
      frameNode
      |> member("children")
      |> to_list
      |> List.map(node => {
           let nodeType = getNodeType(node);
           switch (nodeType) {
           | NodeText => Text(parseText(node))
           | NodeColor => Color(parseColor(node))
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
  };

  let parseCanvas = canvasNode => {
    {
      id: canvasNode |> member("id") |> to_string,
      name: canvasNode |> member("name") |> to_string,
      visible: canvasNode |> member("visible") |> to_bool_option,
      type_: canvasNode |> member("type") |> to_string,
      backgroundColor: canvasNode |> member("backgroundColor") |> parseColor_,
      children:
        canvasNode |> member("children") |> to_list |> List.map(parseFrame),
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
        docNode |> member("children") |> to_list |> List.map(parseCanvas),
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
  switch (canvas.children |> List.find((frame: frame) => frame.name == name)) {
  | v => Some(v)
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
             | Color(c) => c
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
let unitOfKeyAndValue = (css, value) =>
  switch (css, value) {
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