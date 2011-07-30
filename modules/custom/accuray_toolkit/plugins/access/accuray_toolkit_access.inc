<?php
// $Id: example_role.inc,v 1.1.2.2 2010/07/14 01:30:19 merlinofchaos Exp $

/**
 * @file
 * Plugin to provide access control based upon role membership.
 * This is directly from the ctools module, but serves as a good
 * example of an access plugin
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t("Accuray toolkit access"),
  'description' => t('Determine whether the user has access to a toolkit.'),
  'callback' => 'accuray_toolkit_access_check',
  'default' => array('vid' => array()),
  'settings form' => 'accuray_toolkit_access_settings',
  'summary' => 'accuray_toolkit_access_summary',
  'required context' => new ctools_context_required(t('Vocabulary'), 'vocabulary'),
);

/**
 * Settings form for the 'by role' access plugin.
 */
function accuray_toolkit_access_settings(&$form, &$form_state, $conf) {
}

/**
 * Compress the roles allowed to the minimum.
 */
function accuray_toolkit_access_settings_submit(&$form, &$form_state) {
  return $form;
}

/**
 * Check for access.
 */
function accuray_toolkit_access_check($conf, $context) {
  if (empty($context) || empty($context->data) || !isset($context->data->vid)) {
    return FALSE;
  }
  
  // load viewobject
  $view = views_get_view('ToolkitNodeAccess');

  // ensure view exists
  if (!$view) return;

  // set object property to return total rows 
  $view->get_total_rows = true;

  // set display_id
  $view->set_display('Defaults');
  
  // set args
  $view->set_arguments($context->data->vid);

  // execute view
  $view->execute();
  
  $rows = count($view->result);

  return $rows > 0 ? TRUE : FALSE;
}

/**
 * Provide a summary description based upon the checked roles.
 */
function accuray_toolkit_access_summary($conf, $context) {
  return t('Check whether users have access to nodes within this toolkit.');
}
