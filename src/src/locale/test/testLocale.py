#
# Copyright(C) 2014 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or(at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
#

from lmi.test.lmibase import LmiTestCase
from lmi.shell import LMIInstance
import subprocess

lctl_var_nam = ["LANG=", "LC_CTYPE=", "LC_NUMERIC=", "LC_TIME=", "LC_COLLATE=",
    "LC_MONETARY=", "LC_MESSAGES=", "LC_PAPER=", "LC_NAME=", "LC_ADDRESS=",
    "LC_TELEPHONE=", "LC_MEASUREMENT=", "LC_IDENTIFICATION=",
    "VC Keymap", "VC Toggle Keymap",
    "X11 Layout", "X11 Model", "X11 Variant", "X11 Options"]

prov_prop_nam = ["Lang", "LCCtype", "LCNumeric", "LCTime", "LCCollate",
    "LCMonetary", "LCMessages", "LCPaper", "LCName", "LCAddress",
    "LCTelephone", "LCMeasurement", "LCIdentification",
    "VConsoleKeymap", "VConsoleKeymapToggle",
    "X11Layouts", "X11Model", "X11Variant", "X11Options"]

# count of locale variables (LANG, LC_*)
NUM_LOC_VAR = 13

def get_lctl_vals():
    """
    Get all locale and keyboard settings from localectl.
    """
    # invoke localectl command
    lctl_output = subprocess.check_output("localectl")

    # parse output
    lctl_locale = len(lctl_var_nam) * [""]
    for line in lctl_output.split("\n"):
       # locale
       for var in lctl_var_nam:
          if var in line:
              # locale
              if '=' in var:
                  lctl_locale[lctl_var_nam.index(var)] = line.split()[-1][len(var):]
              # keyboard
              else:
                  lctl_locale[lctl_var_nam.index(var)] = line.split()[-1]

    return dict(zip(prov_prop_nam, lctl_locale))

def get_lctl_val(var):
    """
    Get particular variable value from localectl.
    """
    lctl_var_vals = get_lctl_vals()
    return lctl_var_vals[var]

def set_lctl_vals(vals):
    """
    Set all locale and keyboard settings with localectl.
    """
    # set locale
    command = ["localectl", "set-locale"]
    for var in lctl_var_nam:
       if lctl_var_nam.index(var) >= NUM_LOC_VAR:
          break
       command.append(var + vals[prov_prop_nam[lctl_var_nam.index(var)]])
    subprocess.call(command)
    # set vc keyboard
    command = ["localectl", "set-keymap", "--no-convert"]
    command.append(vals["VConsoleKeymap"])
    command.append(vals["VConsoleKeymapToggle"])
    subprocess.call(command)
    # set x11 keyboard
    command = ["localectl", "set-x11-keymap", "--no-convert"]
    command.append(vals["X11Layouts"])
    command.append(vals["X11Model"])
    command.append(vals["X11Variant"])
    command.append(vals["X11Options"])
    subprocess.call(command)

def get_prov_vals(self):
    """
    Get locale and keyboard settings from provider.
    """
    # get locale instance
    inst = self.cim_class.first_instance()

    # get locale settings from provider
    inst_prop_dic = inst.properties_dict()

    # filter properties
    prov_prop_val = {}
    for var in prov_prop_nam:
        prov_prop_val[var] = inst_prop_dic[var]
        if not prov_prop_val[var]:
            prov_prop_val[var] = ""

    return prov_prop_val

def get_prov_val(self, var):
    """
    Get particular variable value from provider
    """
    prov_prop_vals = get_prov_vals(self)
    return prov_prop_vals[var]

class TestLocaleProvider(LmiTestCase):
    """
    TestCase class testing OpenLMI Locale Provider functionality.
    """

    CLASS_NAME = "LMI_Locale"
    original_state = {}

    def setUp(self):
        """
        Save current locale and keyboard settings.
        """
        self.original_state = get_lctl_vals()

    def tearDown(self):
        """
        Restore original locale and keyboard settings.
        """
        set_lctl_vals(self.original_state)

    def test_current_setting(self):
        """
        Compare current locale setting displayed by localectl command
        with values obtained from provider.
        """
        self.assertEquals(get_lctl_vals(), get_prov_vals(self))

    def test_vc_keyboard_method(self):
        """
        Set virtual console kaymap and possibly use convert flag to apply same
        setting for X11 keyboard too, check the setting through localectl.
        """

        # few keymaps...
        keymaps = ["fi", "de", "us", "cz"]
        models = {"fi":"pc105", "de":"pc105", "us":"pc105+inet", "cz":""}
        options = {"fi":"terminate:ctrl_alt_bksp", "de":"terminate:ctrl_alt_bksp",
            "us":"terminate:ctrl_alt_bksp", "cz":""
        }
        # note that convert doesn't work for all keymaps (e.g. not for cz) - localed
        # is not able to find appropriate settings in all cases
        converts = {"fi":True, "de":True, "us":True, "cz":False}

        # get locale instance
        inst = self.cim_class.first_instance()

        for keymap in keymaps:
            # set keymap
            inst.SetVConsoleKeyboard(
                Keymap=keymap,
                KeymapToggle=keymaps[keymaps.index(keymap)-1],
                Convert=converts[keymap]
            )
            # check vc keyboard
            self.assertEquals(keymap, get_lctl_val("VConsoleKeymap"))
            self.assertEquals(
                keymaps[keymaps.index(keymap)-1],
                get_lctl_val("VConsoleKeymapToggle"))
            # check x11 keyboard
            if converts[keymap]:
                # layout should be same as on vc
                self.assertEquals(keymap, get_lctl_val("X11Layouts"))
                # localed also sets model and options to some default value
                self.assertEquals(models[keymap], get_lctl_val("X11Model"))
                self.assertEquals(options[keymap], get_lctl_val("X11Options"))

    def test_x11_keyboard_method(self):
        """
        Set X11 keyboard mapping and possibly use convert flag to apply same
        setting for virtual console keyboard too, check through localectl that
        it is really updated.
        """
        # few layouts with various models, variants and options...
        layouts = ["fi", "de", "us", "us,cz,de"]
        models = {"fi":"pc105", "de":"pc105", "us":"pc105+inet", "us,cz,de":""}
        variants = {"fi":"", "de":"", "us":"", "us,cz,de":"dvorak"}
        options = {"fi":"terminate:ctrl_alt_bksp", "de":"terminate:ctrl_alt_bksp",
            "us":"terminate:ctrl_alt_bksp", "us,cz,de":""
        }
        converts = {"fi":True, "de":True, "us":True, "us,cz,de":False}

        # get locale instance
        inst = self.cim_class.first_instance()

        for layout in layouts:
            # set layout
            inst.SetX11Keyboard(
                Layouts=layout,
                Model=models[layout],
                Variant=variants[layout],
                Options=options[layout],
                Convert=converts[layout]
            )
            # check x11 keyboard
            self.assertEquals(layout, get_lctl_val("X11Layouts"))
            self.assertEquals(models[layout], get_lctl_val("X11Model"))
            self.assertEquals(variants[layout], get_lctl_val("X11Variant"))
            self.assertEquals(options[layout], get_lctl_val("X11Options"))
            # check vc keyboard
            if converts[layout]:
                self.assertEquals(layout, get_lctl_val("VConsoleKeymap"))

    def test_set_locale_method(self):
        """
        Set locale, check through localectl that it is really updated.
        """
        # few locales...
        locales = ["en_US.UTF-8", "cs_CZ.UTF-8", "en_AU.UTF-8",
            "de_DE.UTF-8", "C", "el_GR.UTF-8",
            "es_ES.UTF-8", "he_IL.UTF-8", "ko_KR.UTF-8",
            "da_DK.UTF-8", "de_AT.UTF-8", "hu_HU.UTF-8",
            "pt_BR.UTF-8"
        ]

        # get locale instance
        inst = self.cim_class.first_instance()

        # set locale
        inst.SetLocale(
            Lang=locales[0],
            LCCtype=locales[1],
            LCNumeric=locales[2],
            LCTime=locales[3],
            LCCollate=locales[4],
            LCMonetary=locales[5],
            LCMessages=locales[6],
            LCPaper=locales[7],
            LCName=locales[8],
            LCAddress=locales[9],
            LCTelephone=locales[10],
            LCMeasurement=locales[11],
            LCIdentification=locales[12]
        )

        # check locale
        for locale in locales:
            self.assertEquals(locale, get_lctl_val(prov_prop_nam[locales.index(locale)]))

if __name__ == '__main__':
    # Run the standard unittest.main() function to load and run all tests
    unittest.main()

