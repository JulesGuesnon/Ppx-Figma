open Ppxlib;

let makeModule = (~loc, ~name, content) => {
  {
    pstr_desc:
      Pstr_module({
        pmb_name: {
          txt: name,
          loc,
        },
        pmb_expr: {
          pmod_desc: Pmod_structure(content),
          pmod_loc: loc,
          pmod_attributes: [],
        },
        pmb_attributes: [],
        pmb_loc: loc,
      }),
    pstr_loc: loc,
  };
};

let makePolyVariant = (~loc, ~label, param) => {
  pexp_desc: Pexp_variant(label, param),
  pexp_loc: loc,
  pexp_loc_stack: [],
  pexp_attributes: [],
};

let makeIdent = (~loc, ~label) => {
  pexp_desc: Pexp_ident({txt: Lident(label), loc}),
  pexp_loc: loc,
  pexp_loc_stack: [],
  pexp_attributes: [],
};

let makeString = (~loc, content) => {
  pexp_desc: Pexp_constant(Pconst_string(content, None)),
  pexp_loc: loc,
  pexp_loc_stack: [],
  pexp_attributes: [],
};

let makeInt = (~loc, content) => {
  pexp_desc: Pexp_constant(Pconst_integer(content |> string_of_int, None)),
  pexp_loc: loc,
  pexp_loc_stack: [],
  pexp_attributes: [],
};

let makeFloat = (~loc, content) => {
  pexp_desc: Pexp_constant(Pconst_float(content |> string_of_float, None)),
  pexp_loc: loc,
  pexp_loc_stack: [],
  pexp_attributes: [],
};

let makeFunction = (~loc, ~label, args) => {
  pexp_desc: Pexp_apply(makeIdent(~loc, ~label), args),
  pexp_loc: loc,
  pexp_loc_stack: [],
  pexp_attributes: [],
};