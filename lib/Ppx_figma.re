open Ppxlib;
open Figma.Types;

exception Invalid_argument(string);

type colorsAndTexts = {
  texts: option(list(Figma.Types.text)),
  colors: option(list(Figma.Types.color)),
};

let name = "figma";

let expand = (~loc, ~path as _, args: list(string)) => {
  if (List.length(args) < 2) {
    raise(
      Invalid_argument(
        "Too few argument passed. You need at least to provide the file id and  the token, e.g: [token, fileId]",
      ),
    );
  };

  let token = List.nth(args, 0);
  let fileId = List.nth(args, 1);

  let time =
    switch (List.nth(args, 2)) {
    | time => Time.ofString(time)
    | exception _ => Time.ofString("30m")
    };

  let res =
    Figma.Parser.parse(Figma.Api.getFile(~token, ~fileId, ~time, ()));

  let colorsAndTexts = {
    texts: Figma.getTexts(res),
    colors: Figma.getColors(res),
  };

  Utils.Ast.makeModule(
    ~loc,
    ~name="Styleguide",
    [
      switch (colorsAndTexts.texts) {
      | Some(texts) =>
        texts
        |> List.map((text: text) =>
             text.characters |> Format.parseString(~styles=text.styles)
           )
        |> Format.mergeNodes
        |> List.map(node => Format.nodeToAstNode(~loc, node))
        |> Utils.Ast.makeModule(~loc, ~name="Fonts")
      | None => Utils.Ast.makeModule(~loc, ~name="Fonts", [])
      },
      switch (colorsAndTexts.colors) {
      | Some(colors) =>
        colors
        |> List.map((color: color) => {
             let fill = color.fills |> List.hd;
             let name =
               String.map(
                 char =>
                   if (char == ' ' || char == '-') {
                     '_';
                   } else {
                     char;
                   },
                 color.name,
               );
             let nameFirstChar = name.[0] |> Char.lowercase_ascii;

             [%stri
               let [%p
                 Ast_builder.Default.pvar(
                   ~loc,
                   Printf.sprintf(
                     "%c%s",
                     nameFirstChar,
                     String.sub(name, 1, String.length(name) - 1),
                   ),
                 )
               ] =
                 rgba(
                   [%e
                     Utils.Ast.makeInt(
                       ~loc,
                       int_of_float(fill.color.r *. 255.),
                     )
                   ],
                   [%e
                     Utils.Ast.makeInt(
                       ~loc,
                       int_of_float(fill.color.g *. 255.),
                     )
                   ],
                   [%e
                     Utils.Ast.makeInt(
                       ~loc,
                       int_of_float(fill.color.b *. 255.),
                     )
                   ],
                   [%e Utils.Ast.makeFloat(~loc, fill.color.a)],
                 )
             ];
           })
        |> Utils.Ast.makeModule(~loc, ~name="Colors")
      | None => Utils.Ast.makeModule(~loc, ~name="Colors", [])
      },
    ],
  );
};

let ext =
  Extension.declare(
    name,
    Extension.Context.structure_item,
    Ast_pattern.(single_expr_payload(elist(estring(__)))),
    expand,
  );

let () = Driver.register_transformation(~extensions=[ext], name);