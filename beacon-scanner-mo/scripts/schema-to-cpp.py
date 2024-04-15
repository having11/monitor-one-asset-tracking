#
# Copyright (c) 2023 Particle Industries, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import json
import sys

def load_json_schema(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        return json.load(file)

def to_camel_case(snake_str):
    components = snake_str.split('_')
    return components[0] + ''.join(x.title() for x in components[1:])

def generate_cpp_code(json_schema, search_name):
    constants = []
    enums = []
    structs = []

    def generate_enum(enum_type, enum_values):
        enums.append(f"enum class {enum_type} {{")
        for enum_value in enum_values:
            enums.append(f"    e_{enum_value},")
        enums.append("};\n")

    def generate_struct(struct_name, properties):
        struct_code = [f"struct {struct_name}_t {{"]
        for key, value in properties.items():
            if 'type' not in value:
                continue
            full_key = to_camel_case(key)
            if value['type'] == 'object':
                struct_code.append(f"    {full_key} {full_key.lower()};")
                generate_struct(full_key, value['properties'])
            elif value['type'] == 'string' and 'enum' in value:
                enum_type = f"{struct_name}_{full_key}Type"
                generate_enum(enum_type, value['enum'])
                struct_code.append(f"    {enum_type} {full_key.lower()};")
            elif value['type'] == 'integer':
                struct_code.append(f"    int32_t {full_key.lower()};")
            elif value['type'] == 'number':
                struct_code.append(f"    double {full_key.lower()};")
            elif value['type'] == 'boolean':
                struct_code.append(f"    bool {full_key.lower()};")
        struct_code.append("};\n")
        structs.append('\n'.join(struct_code))

    def recurse(properties, parent_key=''):
        found = False
        for key, value in properties.items():
            if 'type' not in value:
                continue
            full_key = f"{parent_key}_{to_camel_case(key)}" if parent_key else to_camel_case(key)
            if value['type'] == 'object':
                if key == search_name:
                    found = True
                    generate_struct(to_camel_case(key), value['properties'])
                    break
                elif recurse(value['properties'], full_key):
                    found = True
                    break
            elif value['type'] == 'string' and 'enum' in value:
                enum_type = to_camel_case(key)
                generate_enum(enum_type, value['enum'])
                if 'default' in value:
                    constants.append(f"constexpr {enum_type} {full_key.upper()}_DEFAULT = {enum_type}::e_{value['default'].upper()};\n")
            elif value['type'] == 'integer':
                if 'default' in value:
                    constants.append(f"constexpr int32_t {full_key.upper()}_DEFAULT = {value['default']};")
                if 'minimum' in value:
                    constants.append(f"constexpr int32_t {full_key.upper()}_MIN = {value['minimum']};")
                if 'maximum' in value:
                    constants.append(f"constexpr int32_t {full_key.upper()}_MAX = {value['maximum']};")
            elif value['type'] == 'number':
                if 'default' in value:
                    constants.append(f"constexpr double {full_key.upper()}_DEFAULT = {value['default']};")
                if 'minimum' in value:
                    constants.append(f"constexpr double {full_key.upper()}_MIN = {value['minimum']};")
                if 'maximum' in value:
                    constants.append(f"constexpr double {full_key.upper()}_MAX = {value['maximum']};")
            elif value['type'] == 'boolean':
                if 'default' in value:
                    constants.append(f"constexpr bool {full_key.upper()}_DEFAULT = {'true' if value['default'] else 'false'};")
        return found

    recurse(json_schema['properties'], search_name)
    return '\n'.join(enums + constants + structs)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert JSON schema to C++ code")
    parser.add_argument("file_path", help="Path to the JSON schema file")
    parser.add_argument("search_name", help="Root name for the C++ structs")
    args = parser.parse_args()

    try:
        json_schema = load_json_schema(args.file_path)
        cpp_code = generate_cpp_code(json_schema, args.search_name)
        print(cpp_code)
    except FileNotFoundError:
        print("Error: The specified file does not exist.", file=sys.stderr)
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON: {e}", file=sys.stderr)
