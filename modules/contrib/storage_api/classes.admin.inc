<?php
// $Id: classes.admin.inc,v 1.1.2.3.2.13 2010/02/06 22:17:25 jbrown Exp $

// Copyright 2008-2009 Jonathan Brown


function _storage_api_trash_icon() {

  return '<img src="' . base_path() . drupal_get_path('module', 'storage_api') . '/trash.png' . 
    '" style="vertical-align: middle;" />';
}


function _storage_api_class_name($class_id) {

  return db_result(db_query("
    SELECT name
    FROM {storage_class}
    WHERE class_id = %d
  ",
    $class_id
  ));
}


function _storage_api_classes_query() {

  $query = "
    SELECT {storage_class}.*,
      COUNT({storage_file}.file_id) AS files,
      SUM({storage_object}.size) AS size
    FROM {storage_class}
    LEFT JOIN {storage_selector}
      ON {storage_class}.class_id = {storage_selector}.class_id
    LEFT JOIN {storage_file}
      ON {storage_selector}.selector_id = {storage_file}.selector_id
    LEFT JOIN {storage_object}
      ON {storage_file}.object_id = {storage_object}.object_id
    GROUP BY {storage_class}.class_id
  ";
  
  return $query;
}


function storage_api_classes_list() {

  drupal_set_title(t('Storage classes'));

  $query = _storage_api_classes_query();
  $result = db_query($query . "ORDER BY name, class_id");

  $header = array(
    t('Name'), t('Files'), t('Size')
  );

  while($class = db_fetch_array($result)) {
    unset($row);
    
    $row[] = l($class['name'], 'admin/build/storage/classes/' . $class['class_id']);
    $row[] = $class['files'];
    $row[] = storage_api_format_size($class['size']);
    
    $rows[] = $row;
  }

  if(empty($rows)) {
    $rows[] = array(array('data' => t('No classes configured.'), 'colspan' => 3, 'class' => 'message'));
  }

  return theme('table', $header, $rows, array('style' => 'width: auto;'));
}


function _storage_api_class_form_elements($class = NULL) {

  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#default_value' => $class['name']
  );
  
  $form['options'] = array(
    '#tree' => TRUE,
    'access_control' => array(
      '#title' => t('Enforce access control'),
      '#type' => 'checkbox',
      '#default_value' => $class['options']['access_control']
    ),
    'redirect' => array(
      '#title' => t('Serve by redirection'),
      '#type' => 'checkbox',
      '#default_value' => $class['options']['redirect']
    ),
    'serve_source_url' => array(
      '#title' => t('Serve source url'),
      '#type' => 'checkbox',
      '#default_value' => $class['options']['serve_source_url'],
      '#description' => t('If no instances are available, serve the source url.')
    )
  );

  return $form;
}  


/**
 * FAPI definition for the create class form.
 *
 * @ingroup forms
 * @see storage_api_create_class_form_submit()
 */
function storage_api_create_class_form($form_state) {

  drupal_set_title(t('Storage classes'));
  
  $form = _storage_api_class_form_elements(); 

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create class')
  );
  
  return $form;
}


/**
 * Submit handler for the create class form.
 *
 * @see storage_api_create_class_form()
 */
function storage_api_create_class_form_submit($form, &$form_state) {

  $class = array(
    'name' => $form_state['values']['name'],
    'options' => $form_state['values']['options'],
  );
  
  drupal_write_record('storage_class', $class);

  $message = 'Storage class <i>' . $form_state['values']['name'] . '</i> has been created.';
  $path = 'admin/build/storage/classes/' . $class['class_id'];

  drupal_set_message($message);
  watchdog('storage_api', $message, NULL, WATCHDOG_NOTICE, l(t('view'), $path));

  $form_state['redirect'] = $path . '/add';
}


function storage_api_class($class) {

  drupal_set_title(t('Storage class: ') . $class['name']);
  
  $query = _storage_api_classes_query();
  $class_info = db_fetch_array(db_query($query . "HAVING class_id = %d", $class['class_id']));

  $info[t('Files')] = $class_info['files'];
  $info[t('Size')] = storage_api_format_size($class_info['size']);
  $info[t('Access control')] = $class['options']['access_control'] ? t('Yes') : t('No');
  $info[t('Redirection')] = $class['options']['redirect'] ? t('Yes') : t('No');
  $info[t('Serve source')] = $class['options']['serve_source_url'] ? t('Yes') : t('No');
  $info[t('Initial container')] = _storage_api_container_link($class['initial_container']);
  
  $output = theme_storage_api_info_table($info);

  
  $result = db_query("
    SELECT {storage_container}.*, {storage_class_container}.serving AS serving, 
      objects.count AS objects_count, objects.size AS objects_size,
      files.count AS files_count, files.size AS files_size
    FROM {storage_class_container}
    INNER JOIN {storage_container}
      ON {storage_class_container}.container_id = {storage_container}.container_id
    LEFT JOIN (
    
      SELECT {storage_object_instance}.container_id, 
        COUNT({storage_file}.file_id) AS count, SUM({storage_object}.size) AS size
      FROM {storage_selector}
      INNER JOIN {storage_file}
        ON {storage_selector}.selector_id = {storage_file}.selector_id
      INNER JOIN {storage_object}
        ON {storage_file}.object_id = {storage_object}.object_id
      INNER JOIN {storage_object_instance}
        ON {storage_object}.object_id = {storage_object_instance}.object_id
      WHERE {storage_selector}.class_id = %d
      GROUP BY {storage_object_instance}.container_id
      
    ) AS objects
      ON {storage_container}.container_id = objects.container_id
    LEFT JOIN (
    
      SELECT {storage_file_instance}.container_id, 
        COUNT({storage_file}.file_id) AS count, SUM({storage_object}.size) AS size
      FROM {storage_selector}
      INNER JOIN {storage_file}
        ON {storage_selector}.selector_id = {storage_file}.selector_id
      INNER JOIN {storage_object}
        ON {storage_file}.object_id = {storage_object}.object_id
      INNER JOIN {storage_file_instance}
        ON {storage_file}.file_id = {storage_file_instance}.file_id
      WHERE {storage_selector}.class_id = %d
      GROUP BY {storage_file_instance}.container_id
      
    ) AS files
      ON {storage_container}.container_id = files.container_id
    WHERE {storage_class_container}.class_id = %d
    ORDER BY {storage_class_container}.weight, {storage_container}.container_id
  ",
    $class['class_id'], $class['class_id'], $class['class_id']
  );

  while($container = db_fetch_array($result)) {
    unset($row);
    
    $row[] = _storage_api_container_link($container);
    $row[] = l(_storage_api_get_service_name($container['service_id']), 
        'admin/build/storage/services/' . $container['service_id']);
    $row[] = $container['serving'] ? t('Yes') : t('No');
    $row[] = $container['objects_count'] + $container['files_count'];
    $row[] = storage_api_format_size($container['objects_size'] + $container['files_size']);
    
    $rows[] = $row;
  }

  if(empty($rows)) {
    $rows[] = array(array('data' => t('There are no containers in this class.'), 'colspan' => 5, 'class' => 'message'));
  }

  $header = array(t('Container'), t('Service'), t('Serving'), t('Files'), t('Size'));
  $output .= theme('table', $header, $rows, array('style' => 'width: auto;'));

  return $output;
}


/**
 * FAPI definition for the class edit form.
 *
 * @ingroup forms
 * @see storage_api_class_edit_form_submit()
 */
function storage_api_class_edit_form($form_state, $class) {

  drupal_set_title(t('Storage class: ') . $class['name']);

  $form = _storage_api_class_form_elements($class);
  
  $result = db_query("
    SELECT {storage_container}.container_id, {storage_container}.name, {storage_container}.service_id, 
      {storage_class_container}.weight, {storage_class_container}.serving
    FROM {storage_class_container}
    INNER JOIN {storage_container}
      ON {storage_class_container}.container_id = {storage_container}.container_id
    WHERE class_id = %d
    ORDER BY {storage_class_container}.weight, {storage_container}.container_id
  ",
    $class['class_id']
  );

  $form['#tree'] = TRUE;
  
  $trash = _storage_api_trash_icon();

  while($container = db_fetch_array($result)) {
  
    $form['#container_ids'][] = $container['container_id'];

    $form['names'][$container['container_id']] = array(
      '#value' => _storage_api_container_link($container)
    );
    
    $form['weights'][$container['container_id']] = array(
      '#type' => 'weight', 
      '#default_value' => $container['weight'],
      '#attributes' => array('class' => "my-elements-weight")

    );

    $form['services'][$container['container_id']] = array(
      '#value' => l(_storage_api_get_service_name($container['service_id']), 
        'admin/build/storage/services/' . $container['service_id'])
    );
    
    $form['servings'][$container['container_id']] = array(
      '#type' => 'select',
      '#options' => array(
        0 => t('No'),
        1 => t('Yes'),
      ),
      '#default_value' => $container['serving'] ? 1 : 0
    );

    $form['deletes'][$container['container_id']] = array(
      '#value' => 
        l($trash, 'admin/build/storage/classes/' . $class['class_id'] . '/remove/' . $container['container_id'], 
          array('html' => TRUE))
    );
  }

  $result = db_query("
    SELECT *
    FROM {storage_container}
    ORDER BY name, container_id
  ",
    $class['class_id']
  );
  
  while($container = db_fetch_array($result)) {
    $containers[$container['container_id']] = $container['name'];
  }

  $form['initial_container_id'] = array(
    '#type' => 'select',
    '#title' => t('Initial container'),
    '#options' => $containers,
    '#default_value' => $class['options']['initial_container_id'],
    '#description' => t('Files added to the class will initially only have an instance in this container.<br />If the initial container is not in the class, then files\' instances in it will be destroyed once they have been fully propagated.<br />When it is not possible to serve from a container in the class, this container will be used (while instances still exist).<br />Typically this is used to quickly store files locally on the server, providing a faster experience for the user. This is especially important for images rendered in-request.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  $form['destroy'] = array(
    '#type' => 'submit',
    '#value' => t('Destroy'),
    '#submit' => array('storage_api_class_edit_form_destroy')
  );
  
  return $form;
}


/**
 * Themes the class edit form.
 *
 * @ingroup themeable
 * @see storage_api_class_edit_form()
 */
function theme_storage_api_class_edit_form($form) {

  $output .= drupal_render($form['name']);
  $output .= drupal_render($form['options']);

  if(is_array($form['#container_ids'])) {
  
    $header = array(t('Container'), t('Service'), t('Weight'), t('Serving'), NULL);

    foreach($form['#container_ids'] as $container_id) {
      unset($row);
      
      $row[] = drupal_render($form['names'][$container_id]);
      $row[] = drupal_render($form['services'][$container_id]);
      $row[] = drupal_render($form['weights'][$container_id]);
      $row[] = drupal_render($form['servings'][$container_id]);
      $row[] = drupal_render($form['deletes'][$container_id]);
      
      $rows[] = array(
        'data' => $row,
        'class' => 'draggable',
      );
    }
  }
  
  $fieldset = array(
    '#type' => 'fieldset',
    '#title' => t('Class containers'),
    'table' => array(
      '#value' => theme('table', $header, $rows, array('id' => 'my-module-table', 'style' => 'width: auto;')),
    ),
    '#description' => t('<p>Broadly speaking, containers at the top should be nearest the client; those at the bottom should be nearest the server.</p><p>During cron, files are propagated to all the containers in their class (starting at the top). Once this is complete, any instances of files that are not in these containers will be destroyed.</p><p>Files are served from the container nearest the top that has an instance of the file and is capable of serving - it must be a serving container, it must not be suspended, its service must not be suspended and it must also be configured to be serving within the class.</p>'),
  );

  $output .= drupal_render($fieldset);
  
  drupal_add_tabledrag('my-module-table', 'order', 'sibling', 'my-elements-weight');

  $output .= drupal_render($form);
  return $output;
}


/**
 * Submit handler for the class edit form.
 *
 * @see storage_api_class_edit_form()
 */
function storage_api_class_edit_form_submit($form, &$form_state) {
  $class = $form['#parameters'][2];

  $update = array(
    'class_id' => $class['class_id'],
    'name' => $form_state['values']['name'],
    'options' => $form_state['values']['options'],
  );
  
  $update['options']['initial_container_id'] = $form_state['values']['initial_container_id'];

  drupal_write_record('storage_class', $update, 'class_id');
  
  foreach($form['#container_ids'] as $container_id) {

    $update = array(
      'class_id' => $class['class_id'],
      'container_id' => $container_id,
      'weight' => $form_state['values']['weights'][$container_id],
      'serving' => $form_state['values']['servings'][$container_id],
    );

    drupal_write_record('storage_class_container', $update, array('class_id', 'container_id'));
  }
  
  db_query("
    UPDATE {storage_file}
    SET serving_container = NULL
    WHERE selector_id IN (
    
      SELECT selector_id
      FROM {storage_selector}
      WHERE class_id = %d
      
    )
  ",
    $class['class_id']
  );
  
  cache_clear_all();
  
  $message = 'Storage class <i>' . $class['name'] . '</i> has been updated.';
  $path = 'admin/build/storage/classes/' . $class['class_id'];

  drupal_set_message($message);
  watchdog('storage_api', $message, NULL, WATCHDOG_NOTICE, l(t('view'), $path));

  $form_state['redirect'] = $path;
}


/**
 * Submit handler (destroy) for the class edit form.
 *
 * @see storage_api_class_edit_form()
 */
function storage_api_class_edit_form_destroy($form, &$form_state) {
  $class = $form['#parameters'][2];

  $form_state['redirect'] = 'admin/build/storage/classes/' . $class['class_id'] . '/destroy';
}


/**
 * FAPI definition for the class destroy form.
 *
 * @ingroup forms
 * @see storage_api_class_destroy_form_submit()
 */
function storage_api_class_destroy_form($form_state, $class) {

  return confirm_form($form, 'Are you sure you want to destroy storage class <i>' . 
    $class['name'] . '</i> ?', 'admin/build/storage/classes/' . $class['class_id'], t(''));
}


/**
 * Submit handler for the class destroy form.
 *
 * @see storage_api_class_destroy_form()
 */
function storage_api_class_destroy_form_submit($form, &$form_state) {
  $class = $form['#parameters'][2];
  
  db_query("
    DELETE FROM {storage_class}
    WHERE class_id = %d
  ",
    $class['class_id']
  );

  $message = 'Storage class <i>' . _storage_api_class_name($class['class_id']) . '</i> has been destroyed.';

  drupal_set_message($message);
  watchdog('storage_api', $message, NULL);

  $form_state['redirect'] = 'admin/build/storage/classes';
}


/**
 * FAPI definition for the class add container form.
 *
 * @ingroup forms
 * @see storage_api_class_add_container_form_submit()
 */
function storage_api_class_add_container_form($form_state, $class) {

  drupal_set_title(t('Storage class: ') . $class['name']);

  $result = db_query("
    SELECT *
    FROM {storage_container}
    WHERE container_id NOT IN (
    
      SELECT container_id
      FROM {storage_class_container}
      WHERE class_id = %d
      
    )
    ORDER BY name, container_id
  ",
    $class['class_id']
  );
  
  while($container = db_fetch_array($result)) {
    $containers[$container['container_id']] = $container['name'];
  }
  
  if(count($containers) == 0) {
  
    $form[] = array(
      '#value' => '<p>There are no containers left to add to the class.</p>'
    );
    
    return $form;
  }

  $form['container_id'] = array(
    '#title' => t('Container'),
    '#type' => 'select',
    '#required' => TRUE,
    '#options' => $containers
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add')
  );

  return $form;
}


/**
 * Submit handler for the class add container form.
 *
 * @see storage_api_class_add_container_form()
 */
function storage_api_class_add_container_form_submit($form, &$form_state) {
  $class = $form['#parameters'][2];
  $container = storage_api_container_load($form_state['values']['container_id']);
  
  storage_api_class_add_container($class, $container);
  
  $form_state['redirect'] = 'admin/build/storage/classes/' . $class['class_id'] . '/edit';
}


/**
 * FAPI definition for the class remove container form.
 *
 * @ingroup forms
 * @see storage_api_class_remove_container_form_submit()
 */
function storage_api_class_remove_container_form($form_state, $class, $container) {

  $form['#class_id'] = $class['class_id'];
  $form['#container_id'] = $container['container_id'];

  return confirm_form($form, 'Are you sure you want to remove storage container <i>' . $container['name'] . 
    '</i> from class <i>' . $class['name'] . '</i>?', 
    'admin/build/storage/classes/' . $class['class_id'] . '/edit', t(''));
}


/**
 * Submit handler for the class remove container form.
 *
 * @see storage_api_class_remove_container_form()
 */
function storage_api_class_remove_container_form_submit($form, &$form_state) {
  $class = $form['#parameters'][2];
  $container = $form['#parameters'][3];

  db_query("
    DELETE FROM {storage_class_container}
    WHERE class_id = %d
      AND container_id = %d
  ",
    $class['class_id'],
    $container['container_id']
  );
  
  db_query("
    UPDATE {storage_file}
    SET check_me = 1,
      serving_container = NULL
    WHERE selector_id IN (
    
      SELECT selector_id
      FROM {storage_selector}
      WHERE class_id = %d
      
    )
  ",
    $class['class_id']
  );
  
  cache_clear_all();

  $container = storage_api_container_load($container['container_id']);
  $path = 'admin/build/storage/classes/' . $class['class_id'];
  
  $message = 'Storage container <i>' . $container['name'] . 
    '</i> has been removed from class <i>' . $class['name'] .'</i>.';
    
  drupal_set_message($message);
  watchdog('storage_api', $message, NULL, WATCHDOG_NOTICE, l(t('view'), $path));
  
  $form_state['redirect'] = $path;
}

