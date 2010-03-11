<?php
// $Id$

/**
 * @file
 * All logic for options_element form elements.
 */


/**
 * Theme an options element.
 */
function theme_options($element) {
  drupal_add_js('misc/tabledrag.js');
  drupal_add_js(drupal_get_path('module', 'options_element') .'/options_element.js');
  drupal_add_css(drupal_get_path('module', 'options_element') .'/options_element.css');

  $classes = array();
  if (isset($element['#attributes']['class'])) {
    $classes[] = $element['#attributes']['class'];
  }

  $classes[] = 'form-options';
  $classes[] = 'options-key-type-'. $element['#key_type'];

  if ($element['#key_type_toggled']) {
    $classes[] = 'options-key-custom';
  }

  if (isset($element['#optgroups']) && $element['#optgroups']) {
    $classes[] = 'options-optgroups';
  }

  if (isset($element['#multiple']) && $element['#multiple']) {
    $classes[] = 'options-multiple';
  }

  $options = '';
  $options .= drupal_render($element['options_field']);
  $options .= drupal_render($element['default_value_field']);

  $settings = '';
  if (isset($element['custom_keys'])) {
    $settings .= drupal_render($element['custom_keys']);
  }
  if (isset($element['multiple'])) {
    $settings .= drupal_render($element['multiple']);
  }

  $output = '';
  $output .= '<div class="' . implode(' ', $classes) .'">';
  $output .= theme('fieldset', array(
    '#title' => t('Options'),
    '#collapsible' => FALSE,
    '#children' => $options,
    '#attributes' => array('class' => 'options'),
  ));

  if (!empty($settings)) {
    $output .= theme('fieldset', array(
      '#title' => t('Option settings'),
      '#collapsible' => FALSE,
      '#children' => $settings,
      '#attributes' => array('class' => 'option-settings'),
    ));
  }
  $output .= '</div>';

  return $output;
}

/**
 * Logic function for form_options_expand(). Do not call directly.
 *
 * @see form_options_expand()
 */
function _form_options_expand($element) {
  $element['#options'] = isset($element['#options']) ? $element['#options'] : array();
  $element['#multiple'] = isset($element['#multiple']) ? $element['#multiple'] : FALSE;

  $element['#tree'] = TRUE;
  $element['#theme'] = 'options';

  // Add the key type toggle checkbox.
  if (!isset($element['custom_keys']) && $element['#key_type'] != 'custom' && !empty($element['#key_type_toggle'])) {
    $element['custom_keys'] = array(
      '#title' => is_string($element['#key_type_toggle']) ? $element['#key_type_toggle'] : t('Customize keys'),
      '#type' => 'checkbox',
      '#default_value' => $element['#key_type_toggled'],
      '#attributes' => array('class' => 'key-type-toggle'),
      '#description' => t('Customizing the keys will allow you to save one value internally while showing a different option to the user.'),
    );
  }

  // Add the multiple value toggle checkbox.
  if (!isset($element['multiple']) && !empty($element['#multiple_toggle'])) {
    $element['multiple'] = array(
      '#title' => is_string($element['#multiple_toggle']) ? $element['#multiple_toggle'] : t('Allow multiple values'),
      '#type' => 'checkbox',
      '#default_value' => !empty($element['#multiple']),
      '#attributes' => array('class' => 'multiple-toggle'),
      '#description' => t('Multiple values will let users select multiple items in this list.'),
    );
  }

  // Add the main textfield for adding options.
  if (!isset($element['options'])) {
    $element['options_field'] = array(
      '#type' => 'textarea',
      '#resizable' => TRUE,
      '#cols' => 60,
      '#rows' => 5,
      '#required' => isset($element['#required']) ? $element['#required'] : FALSE,
      '#description' => t('List options one option per line.'),
    );

    // If validation fails, reload the user's text even if it's not valid.
    if (isset($element['#value']['text'])) {
      $element['options_field']['#value'] = $element['#value']['text'];
    }
    // Most of the time, we'll be converting the options array into the text.
    else {
      $element['options_field']['#value'] = isset($element['#options']) ? form_options_to_text($element['#options'], $element['#key_type']) : '';
    }


    if ($element['#key_type'] == 'mixed' || $element['#key_type'] == 'numeric' || $element['#key_type'] == 'custom') {
      $element['options_field']['#description'] .= ' ' . t('Key-value pairs may be specified by separating each option with pipes, such as <em>key|value</em>.');
    }

    if ($element['#key_type_toggle']) {
      $element['options_field']['#description'] .= ' ' . t('If the %toggle field is checked, key-value pairs may be specified by separating each option with pipes, such as <em>key|value</em>.', array('%toggle' => $element['custom_keys']['#title']));
    }

    if ($element['#key_type'] == 'numeric') {
      $element['options_field']['#description'] .= ' ' . t('This field requires all specified keys to be integers.');
    }
  }

  // Add the field for storing default values.
  if (!isset($element['default_value_field'])) {
    $element['default_value_field'] = array(
      '#title' => t('Default value'),
      '#type' => 'textfield',
      '#size' => 60,
      '#value' => isset($element['#default_value']) ? ($element['#multiple'] ? implode(', ', (array) $element['#default_value']) : $element['#default_value']) : '',
      '#description' => t('Specify the keys that should be selected by default.'),
    );
    if ($element['#multiple']) {
      $element['default_value_field']['#description'] .= ' ' . t('Multiple default values may be specified by separating keys with commas.');
    }
  }

  // Remove properties that will confuse the FAPI.
  unset($element['#options']);
  $element['#required'] = FALSE;

  return $element;
}

/**
 * Logic function for form_options_validate(). Do not call directly.
 *
 * @see form_options_validate()
 */
function _form_options_validate($element, &$form_state) {
  // Convert text to an array of options.
  $duplicates = array();
  $options = form_options_from_text($element['#value']['text'], $element['#key_type'], empty($element['#optgroups']), $duplicates);

  // Check if a key is used multiple times.
  if (count($duplicates) == 1) {
    form_error($element, t('The key %key has been used multiple times. Each key must be unique to display properly.', array('%key' => reset($duplicates))));
  }
  elseif (!empty($duplicates)) {
    array_walk($duplicates, 'check_plain');
    $duplicate_list = theme('item_list', $duplicates);
    form_error($element, t('The following keys have been used multiple times. Each key must be unique to display properly.') . $duplicate_list);
  }

  // Add the list of duplicates to the page so that we can highlight the fields.
  if (!empty($duplicates)) {
    drupal_add_js(array('optionsElement' => array('errors' => $duplicates)), 'setting');
  }

  // Check if no options are specified.
  if (empty($options) && $element['#required']) {
    form_error($element, t('At least one option must be specified.'));
  }

  // Check for numeric keys if needed.
  if ($element['#key_type'] == 'numeric') {
    foreach ($options as $key => $value) {
      if (!is_int($key)) {
        form_error($element, t('The keys for the %title field must be integers.', array('%title' => $element['#title'])));
        break;
      }
    }
  }

  // Check that the limit of options has not been exceeded.
  if (!empty($element['#limit'])) {
    $count = 0;
    foreach ($options as $value) {
      if (is_array($value)) {
        $count += count($value);
      }
      else {
        $count++;
      }
    }
    if ($count > $element['#limit']) {
      form_error($element, t('The %title field supports a maximum of @count options. Please reduce the number of options.', array('%title' => $element['#title'], '@count' => $element['#limit'])));
    }
  }
}

/**
 * Logic function for form_type_options_value(). Do not call directly.
 *
 * @see form_type_options_value()
 */
function _form_type_options_value(&$element, $edit = FALSE) {
  if ($edit === FALSE) {
     return array(
       'options' => isset($element['#options']) ? $element['#options'] : array(),
       'default_value' => isset($element['#default_value']) ? $element['#default_value'] : '',
     );
  }
  else {
    // Convert text to an array of options.
    $duplicates = array();
    $options = form_options_from_text($edit['options_field'], $element['#key_type'], empty($element['#optgroups']), $duplicates);

    // Convert default value.
    if ($element['#multiple']) {
      $default_value = explode(',', $edit['default_value_field']);
      foreach ($default_value as $key => &$value) {
        $value = trim($value);
        if ($value === '') {
          unset($default_value[$key]);
        }
      }
    }
    else {
      $default_value = $edit['default_value_field'];
    }

    return array(
      'options' => $options,
      'text' => $edit['options_field'],
      'default_value' => $default_value,
    );
  }
}

/**
 * Logic function for form_options_to_text(). Do not call directly.
 *
 * @see form_options_to_text()
 */
function _form_options_to_text($options, $key_type) {
  $output = '';
  $previous_key = false;
  $all_options = array();

  foreach ($options as $key => $value) {
    // Convert groups.
    if (is_array($value)) {
      $output .= '<' . $key . '>' . "\n";
      foreach ($value as $subkey => $subvalue) {
        $output .= (($key_type == 'mixed' || $key_type == 'numeric' || $key_type == 'custom') ? $subkey . '|' : '') . $subvalue . "\n";
      }
      $previous_key = $key;
    }
    // Typical key|value pairs.
    else {
      // Exit out of any groups.
      if (isset($options[$previous_key]) && is_array($options[$previous_key])) {
        $output .= "<>\n";
      }
      // Skip empty rows.
      if ($options[$key] !== '') {
        if ($key_type == 'mixed' || $key_type == 'numeric' || $key_type == 'custom') {
          $output .= $key . '|' . $value . "\n";
        }
        else {
          $output .= $value . "\n";
        }
      }
      $previous_key = $key;
    }
  }

  return $output;
}

/**
 * Logic function for form_options_from_text(). Do not call directly.
 *
 * @see form_options_from_text()
 */
function _form_options_from_text($text, $key_type, $flat = FALSE, &$duplicates = array()) {
  $options = array();
  $rows = array_filter(explode("\n", trim($text)));
  $group = FALSE;

  foreach ($rows as $option) {
    $option = trim($option);
    $matches = array();

    // Check for a simple empty row.
    if (empty($option)) {
      continue;
    }
    // Check if this row is a group.
    elseif (!$flat && preg_match('/^\<([^>]*)\>$/', $option, $matches)) {
      if ($matches[1] === '') {
        $group = FALSE;
      }
      elseif (!$flat) {
        $group = $matches[1];
      }
    }
    // Check if this row is a key|value pair.
    elseif (($key_type == 'mixed' || $key_type == 'custom' || $key_type == 'numeric') && preg_match('/^([^|]+)\|(.*)$/', $option, $matches)) {
      $key = $matches[1];
      $value = $matches[2];
      if ($group !== FALSE) {
        isset($options[$group][$key]) ? $duplicates[$key] = $key : $options[$group][$key] = $value;
      }
      else {
        isset($options[$key]) ? $duplicates[$key] = $key : $options[$key] = $value;
      }
    }
    // Check if this row is a straight value.
    else {
      if ($group !== FALSE) {
        if ($key_type == 'none' || $key_type == 'mixed') {
          $options[$group][] = $option;
        }
        else {
          isset($options[$group][$option]) ? $duplicates[$option] = $option : $options[$group][$option] = $option;
        }
      }
      else {
        if ($key_type == 'none' || $key_type == 'mixed') {
          $options[] = $option;
        }
        else {
          isset($options[$option]) ? $duplicates[$option] = $option : $options[$option] = $option;
        }
      }
    }
  }

  return $options;
}