<?php
/**
 * Field handler to present a link to delete a node.
 */
class views_handler_field_node_link_delete extends views_handler_field_node_link {
  function construct() {
    parent::construct();
    $this->additional_fields['type'] = 'type';
    $this->additional_fields['uid'] = 'uid';
    $this->additional_fields['format'] = array('table' => 'node_revisions', 'field' => 'format');
  }

  function render_link($data, $values) {
    // ensure user has access to edit this node.
    $node = new stdClass();
    $node->nid = $this->get_value($values, 'nid');
    $node->uid = $this->get_value($values, 'uid');
    $node->type = $this->get_value($values, 'type');
    $node->format = $this->get_value($values, 'format');
    $node->status = 1; // unpublished nodes ignore access control
    if (!node_access('delete', $node)) {
      return;
    }

    $this->options['alter']['make_link'] = TRUE;
    $this->options['alter']['path'] = "node/$node->nid/delete";
    $this->options['alter']['query'] = drupal_get_destination();

    $text = !empty($this->options['text']) ? $this->options['text'] : t('delete');
    return $text;
  }
}

