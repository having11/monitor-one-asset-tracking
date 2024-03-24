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
    
def parse_schema(json_schema, delim=',', parent_key='', parent_version=''):
    rows = []
    for key, value in json_schema.get('properties', {}).items():
        full_key = f"{parent_key}/{key}" if parent_key else key
        version = value.get('minimumFirmwareVersion', parent_version)
        if value.get('type') == 'object':
            rows.extend(parse_schema(value, delim, full_key, version))
        else:
            type_ = value.get('type', '')
            if delim == ',':
              description = '"' + value.get('description', '').replace('\n', ' ').replace('\t', '    ') + '"'
            else:
              description = value.get('description', '').replace('\n', ' ').replace('\t', '    ')
            rows.append(f"{full_key}{delim}{type_}{delim}{version}{delim}{description}")
    return rows

def save_to_file(rows, file=None):
    for row in rows:
        file.write(row + "\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert JSON schema to CSV")
    parser.add_argument("input", help="Input file name")
    parser.add_argument("--output", help="Output file name (prints to stdout if not provided)")
    parser.add_argument("-t", "--tab", help="Use tab delimiter (TSV) instead of comma (CSV)", action='store_true')
    args = parser.parse_args()

    delim = '\t' if args.tab else ','

    try:
        json_schema = load_json_schema(args.input)
        rows = ["Name" + delim + "Type" + delim + "Version" + delim + "Description"]
        rows.extend(parse_schema(json_schema, delim))

        if args.output:
            with open(args.output, 'w', encoding='utf-8') as file:
                save_to_file(rows, file)
            print("Conversion completed successfully! Output written to:", args.output, file=sys.stdout)
        else:
            save_to_file(rows, sys.stdout)

    except FileNotFoundError:
        print("Error: The specified file does not exist.", file=sys.stderr)
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON: {e}", file=sys.stderr)
