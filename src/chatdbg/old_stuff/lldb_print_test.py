
@lldb.command("print-test")
def print_test(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    """print all variables in a run while recursing through pointers, keeping track of seen addresses"""

    args = command.split()
    recurse_max = 3
    help_string = "Usage: print-test [recurse_max]\n\nrecurse_max: The maximum number of times to recurse through nested structs or pointers to pointers. Default: 3"
    if len(args) > 1 or (len(args) == 1 and args[0] == "--help"):
        print(help_string)
        return
    elif len(args) == 1:
        try:
            recurse_max = int(args[0])
        except ValueError as e:
            print("recurse_max value could not be parsed: %s\n" % args[0])
            return
        if recurse_max < 1:
            print("recurse_max value must be at least 1.\n")
            return
    frame = (
        lldb.debugger.GetSelectedTarget()
        .GetProcess()
        .GetSelectedThread()
        .GetSelectedFrame()
    )

    all_vars = []
    addresses = {}
    for var in frame.get_all_variables():
        # Returns python dictionary for each variable, converts to JSON
        variable = _val_to_json(var, recurse_max, addresses)
        js = json.dumps(variable, indent=4)
        all_vars.append(js)

    # Print all addresses and JSON objects
    # print(addresses)
    for j in all_vars:
        print(j)
    return


def _val_to_json(
    var: lldb.SBValue,
    recurse_max: int,
    address_book: dict,
) -> dict:
    # Store address
    address_book.setdefault(str(var.GetAddress()), var.GetName())

    json = {}
    json["name"] = var.GetName()
    json["type"] = var.GetTypeName()
    # Dereference pointers
    if "*" in var.GetType().GetName():
        if var.GetValueAsUnsigned() != 0:
            value = "->"
            try:
                deref_val = var.Dereference()
                # If dereferenced value is "seen", then get name from address book
                if str(deref_val.GetAddress()) in address_book:
                    json["value"] = address_book[str(deref_val.GetAddress())]
                else:
                    # Recurse up to max_recurse times
                    for i in range(recurse_max - 1):
                        if "*" in deref_val.GetType().GetName():
                            value += "->"
                            deref_val = deref_val.Dereference()
                        elif len(deref_val.GetType().get_fields_array()) > 0:
                            value = _val_to_json(
                                deref_val,
                                recurse_max - i - 1,
                                address_book,
                            )
                            break
                        else:
                            break
                    # Append to -> string or not, depending on type of value
                    if isinstance(value, dict):
                        json["value"] = value
                    else:
                        json["value"] = (
                            value + str(deref_val)[str(deref_val).find("= ") + 2 :]
                        )
            except Exception as e:
                json["value"] = value + "Exception"
        else:
            json["value"] = "nullptr"
    # Recurse through struct fields
    elif len(var.GetType().get_fields_array()) > 0:
        fields = []
        for i in range(var.GetNumChildren()):
            f = var.GetChildAtIndex(i)
            fields.append(
                _val_to_json(
                    f,
                    recurse_max - 1,
                    address_book,
                )
            )
        json["value"] = fields
    else:
        json["value"] = str(var)[str(var).find("= ") + 2 :]
    return json
