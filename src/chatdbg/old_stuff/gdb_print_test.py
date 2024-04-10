# class PrintTest(gdb.Command):
#     """print all variables in a run while recursing through pointers, keeping track of seen addresses"""

#     def __init__(self):
#         super().__init__("print-test", gdb.COMMAND_DATA, gdb.COMPLETE_SYMBOL, True)

#     def invoke(self, arg, from_tty):
#         help_string = "Usage: print-test [recurse_max]\n\nrecurse_max: The maximum number of times to recurse through nested structs or pointers to pointers. Default: 3"
#         if arg == "--help":
#             print(help_string)
#             return
#         recurse_max = 3
#         if arg != "":
#             try:
#                 recurse_max = int(arg)
#             except ValueError as e:
#                 print(f"recurse_max value could not be parsed: {e}")
#                 return
#         if recurse_max < 1:
#             print("recurse_max value must be at least 1.")
#             return
#         frame = gdb.selected_frame()
#         block = gdb.block_for_pc(frame.pc())

#         all_vars = []
#         addresses = {}
#         for symbol in block:
#             if symbol.is_argument or symbol.is_variable:
#                 sym_val = frame.read_var(symbol)
#                 # Returns python dictionary for each variable
#                 variable = self._val_to_json(
#                     symbol.name, sym_val, recurse_max, addresses
#                 )
#                 js = json.dumps(variable, indent=4)
#                 all_vars.append(js)

#         # Print all addresses and JSON objects
#         # print(addresses)
#         for j in all_vars:
#             print(j)

#     # Converts a gdb.Value to a JSON object
#     def _val_to_json(self, name, val, max_recurse, address_book):
#         # Store address
#         address_book.setdefault(str(val.address.format_string()), name)

#         diction = {}
#         # Set var name
#         diction["name"] = name
#         # Set var type
#         if val.type.code is gdb.TYPE_CODE_PTR:
#             diction["type"] = "pointer"  # Default type name is "none"
#         elif val.type.code is gdb.TYPE_CODE_ARRAY:
#             diction["type"] = "array"  # Default type name is "none"
#         else:
#             diction["type"] = val.type.name
#         # Dereference pointers
#         if val.type.code is gdb.TYPE_CODE_PTR:
#             if val:
#                 value = "->"
#                 try:
#                     deref_val = val.referenced_value()
#                     # If dereferenced value is "seen", then get name from address book
#                     if deref_val.address.format_string() in address_book:
#                         diction["value"] = address_book[
#                             deref_val.address.format_string()
#                         ]
#                     else:
#                         # Recurse up to max_recurse times
#                         for i in range(max_recurse - 1):
#                             if deref_val.type.code is gdb.TYPE_CODE_PTR:
#                                 value += "->"
#                                 deref_val = deref_val.referenced_value()
#                             elif deref_val.type.code is gdb.TYPE_CODE_STRUCT:
#                                 value = self._val_to_json(
#                                     value + name,
#                                     deref_val,
#                                     max_recurse - i - 1,
#                                     address_book,
#                                 )
#                                 break
#                             else:
#                                 break
#                         # Append to -> string or not, depending on type of value
#                         if isinstance(value, dict):
#                             diction["value"] = value
#                         else:
#                             diction["value"] = value + deref_val.format_string()
#                 except Exception as e:
#                     diction["value"] = value + "Exception"
#             else:
#                 # Nullptr case, might be a better way to represent
#                 diction["value"] = "nullptr"
#         # If struct, recurse through fields
#         elif val.type.code is gdb.TYPE_CODE_STRUCT:
#             fields = []
#             for f in val.type.fields():
#                 fields.append(
#                     self._val_to_json(
#                         f.name, val[f.name], max_recurse - 1, address_book
#                     )
#                 )
#             diction["value"] = fields
#         else:
#             diction["value"] = val.format_string()
#         return diction


# PrintTest()
