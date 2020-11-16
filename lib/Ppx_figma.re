/* open Migrate_parsetree; */
/* open Ast_410; */
open Ppxlib;
open Figma.Types;

exception Invalid_argument(string);

type stylesToGenerate = {
  texts: option(list(Figma.Types.text)),
  colors: option(list((string, Figma.Types.paints))),
  components: option(list(Figma.Types.component)),
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

  let stylesToGenerate = {
    texts: Figma.getTexts(res),
    colors: Figma.getColors(res),
    components: Some(Figma.getComponents(res)),
  };

  Utils.Ast.makeModule(
    ~loc,
    ~name="Styleguide",
    [
      switch (stylesToGenerate.texts) {
      | Some(texts) =>
        texts
        |> Format.textNodesToAst(~loc)
        |> Utils.Ast.makeModule(~loc, ~name="Fonts")
      | None => Utils.Ast.makeModule(~loc, ~name="Fonts", [])
      },
      switch (stylesToGenerate.colors) {
      | Some(colors) =>
        colors
        |> Format.colorNodesToAst(~loc)
        |> Utils.Ast.makeModule(~loc, ~name="Colors")
      | None => Utils.Ast.makeModule(~loc, ~name="Colors", [])
      },
      /* switch (stylesToGenerate.components) {
         | Some(c) =>
           c
           |> Format.componentNodesToAst(~loc)
           |> Utils.Ast.makeModule(~loc, ~name="Components")
         | None => Utils.Ast.makeModule(~loc, ~name="Components", [])
         }, */
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