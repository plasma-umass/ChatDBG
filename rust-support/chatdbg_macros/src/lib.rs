use proc_macro::TokenStream;
use quote::quote;
use syn::{parse, ItemFn};

#[proc_macro_attribute]
pub fn main(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let ItemFn {
        attrs,
        vis,
        sig,
        block,
    } = parse(item).unwrap();
    let stmts = &block.stmts;
    quote! {
        #(#attrs)* #vis #sig {
            ::chatdbg::chatdbg();
            #(#stmts)*
        }
    }
    .into()
}
