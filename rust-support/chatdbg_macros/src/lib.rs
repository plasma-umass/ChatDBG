use proc_macro::TokenStream;
use quote::quote;
use syn::{parse, ItemFn};

#[proc_macro_attribute]
pub fn main(_attr: TokenStream, input: TokenStream) -> TokenStream {
    let item = match parse(input) {
        Ok(i) => i,
        Err(_) => return quote! {}.into(),
    };
    let ItemFn {
        attrs,
        vis,
        sig,
        block,
    } = item;
    let stmts = &block.stmts;
    quote! {
        #(#attrs)* #vis #sig {
            ::chatdbg::chatdbg();
            #(#stmts)*
        }
    }
    .into()
}
