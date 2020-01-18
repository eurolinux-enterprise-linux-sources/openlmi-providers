export LMI_CIMOM_URL="http://localhost:5988"
export LMI_CIMOM_PASSWORD="pass"
export LMI_CIMOM_USERNAME="pegasus"
export LMI_CIMOM_BROKER="tog-pegasus"
#python -m unittest -v testLocale.TestLocaleProvider.test_current_setting
#python -m unittest -v testLocale.TestLocaleProvider.test_vc_keyboard_method
#python -m unittest -v testLocale.TestLocaleProvider.test_x11_keyboard_method
#python -m unittest -v testLocale.TestLocaleProvider.test_set_locale_method
nosetests -v
