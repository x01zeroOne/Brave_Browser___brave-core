# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
from pathlib import Path
import os

leo_overrides = None
def maybe_load_overrides():
  """Loads the Leo Overrides into the global `leo_overrides` variable"""
  global leo_overrides
  if leo_overrides is not None: return leo_overrides
  brave_root = f'{Path(__file__).parent}/../../brave'
  overrides_file_path =  f'{brave_root}/vector_icons/leo_overrides.json'

  leo_overrides = {}
  data = json.load(open(overrides_file_path))

  for icon, leo_override in data.items():
    leo_overrides[icon] = f'../../brave/node_modules/@brave/leo/icons-skia/{leo_override}.icon'

def get_icon_path(icon_name, icon_path, alt_working_directory):
  """Determines where to read the icon from. Options are:
    1. The Leo `icons-skia` folder, mapping `icon_name` to a Leo icon, as per
       the `leo_overrides.json` file.
    2. The Brave equivalent of the the Chromium icon path.
    3. The Chromium Path (this is the default, if no override is specified).

  Args:
      icon_name: The name of the icon.
      icon_path: The path to the chromium icon
      alt_working_directory: The Brave overrides folder for this icon.
  """
  global leo_overrides
  maybe_load_overrides()

  # Check for alternative path
  alt_icon_path = os.path.join(alt_working_directory, os.path.basename(icon_path))

  if icon_name in leo_overrides: return leo_overrides[icon_name]
  if os.path.exists(alt_icon_path): return alt_icon_path

  return icon_path
