open Ppxlib;
open Figma.Types;

module Types = {
  type nodeType =
    | Var
    | Module;

  type node = {
    name: string,
    type_: nodeType,
    mutable children: list(node),
    mutable merged: bool,
    styles: option(list((string, Figma.Types.primitive))),
  };
};

open Types;

let formatString = (type_: nodeType, str) => {
  let cleanedStr =
    String.map(
      char =>
        if (char == ' ' || char == '-') {
          '_';
        } else {
          char;
        },
      str,
    );

  switch (type_) {
  | Module => cleanedStr |> String.capitalize_ascii
  | Var =>
    let firstChar = cleanedStr.[0] |> Char.lowercase_ascii;
    Printf.sprintf(
      "%c%s",
      firstChar,
      String.sub(cleanedStr, 1, String.length(cleanedStr) - 1),
    );
  };
};

let rec parseString = (~styles, string) => {
  let parts = String.split_on_char('/', string);

  let name = List.hd(parts);

  let type_ =
    if (List.length(parts) == 1) {
      Var;
    } else {
      Module;
    };

  {
    name: formatString(type_, name),
    type_,
    children:
      if (List.length(parts) > 1) {
        [
          parts
          |> List.filter(str => str != name)
          |> String.concat("/")
          |> parseString(~styles),
        ];
      } else {
        [];
      },
    merged: false,
    styles:
      if (type_ == Var) {
        Some(styles);
      } else {
        None;
      },
  };
};

let rec mergeNodes = nodes => {
  nodes |> List.iter(node => node.merged = false);

  let newNodes = ref([]);

  let max = List.length(nodes) - 1;
  for (i in 0 to max) {
    let currentNode = List.nth(nodes, i);

    if (currentNode.type_ == Module && !currentNode.merged) {
      for (j in i + 1 to max) {
        let otherNode = List.nth(nodes, j);
        if (currentNode.name == otherNode.name) {
          otherNode.merged = true;

          currentNode.children =
            mergeNodes(
              List.merge(
                (_, _) => 0,
                currentNode.children,
                otherNode.children,
              ),
            );
        };
      };

      newNodes := [currentNode, ...newNodes^];
    } else if (currentNode.type_ == Var && !currentNode.merged) {
      currentNode.merged = true;
      newNodes := [currentNode, ...newNodes^];
    };
  };

  newNodes^;
};

let rec nodeToAstNode = (~loc, node) =>
  if (node.type_ == Module) {
    Utils.Ast.makeModule(
      ~loc,
      ~name=node.name,
      node.children |> List.map(nodeToAstNode(~loc)),
    );
  } else {
    let styles =
      switch (node.styles) {
      | Some(s) => s
      | None => []
      };

    [%stri
      let [%p Ast_builder.Default.pvar(node.name, ~loc)] =
        /* open of the style function */
        style(
          [%e
            Ast_builder.Default.elist(
              ~loc,
              List.map(
                ((key, value)) => {
                  let expr = Figma.unitOfKeyAndValue(key, value);

                  %expr
                  [%e Ast_builder.Default.evar(~loc, key)]([%e expr(~loc)]);
                },
                styles,
              ),
            )
          ],
        )
    ];
  };

let textNodesToAst = (~loc, texts) => {
  texts
  |> List.map((text: text) =>
       text.characters |> parseString(~styles=text.styles)
     )
  |> mergeNodes
  |> List.map(node => nodeToAstNode(~loc, node));
};

let colorNodesToAst = (~loc, colors) => {
  colors
  |> List.map(((name, fills: paints)) => {
       let fill = fills |> List.hd;

       [%stri
         let [%p Ast_builder.Default.pvar(~loc, formatString(Var, name))] =
           rgba(
             [%e
               Utils.Ast.makeInt(~loc, int_of_float(fill.color.r *. 255.))
             ],
             [%e
               Utils.Ast.makeInt(~loc, int_of_float(fill.color.g *. 255.))
             ],
             [%e
               Utils.Ast.makeInt(~loc, int_of_float(fill.color.b *. 255.))
             ],
             [%e Utils.Ast.makeFloat(~loc, fill.color.a)],
           )
       ];
     });
};

let styleOfComponent = component => {};

let componentNodesToAst = (~loc, components) => {
  let styles = [];

  components
  |> List.map((component: component) => {
       Utils.Ast.makeModule(
         ~loc,
         ~name=formatString(Module, component.name),
         [
           [%stri
             let [%p Ast_builder.Default.pvar(~loc, "root")] =
               style(
                 [%e
                   Ast_builder.Default.elist(
                     ~loc,
                     List.map(
                       ((key, value)) => {
                         let expr = Figma.unitOfKeyAndValue(key, value);

                         %expr
                         [%e Ast_builder.Default.evar(~loc, key)](
                           [%e expr(~loc)],
                         );
                       },
                       styles,
                     ),
                   )
                 ],
               )
           ],
           /* ...component.children |> List.map(a => a), */
         ],
       )
     });
};