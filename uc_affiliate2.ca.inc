<?php
// $Id$

/**
 * @file
 * This file contains the Conditional Actions hooks and functions necessary to make the
 * affiliate-related entity, conditions, events, and actions work.
 */

/******************************************************************************
 * Conditional Actions Hooks                                                  *
 ******************************************************************************/

/**
 * Implementation of hook_ca_entity().
 *
 * An entity is defined in order to get the recurring fee down to token in the
 * email.
 */
function uc_affiliate2_ca_entity() {
  // CA entity for a affiliate commission.
  $entities['uc_affiliate2_commission'] = array(
    '#title' => t('Affiliate Commission'),
    '#type' => 'object',
  );

  return $entities;
}

/**
 * Implementation of hook_ca_predicate().
 */
function uc_affiliate2_ca_predicate() {
  $predicates = array();

  $predicates['uc_affiliate2_apply_commission'] = array(
    '#title' => t('Commission being applied.'),
    '#trigger' => 'uc_affiliate2_apply_commission',
    '#class' => 'affiliate',
    '#status' => 0,
    '#weight' => 2,
    '#conditions' => array(
      '#operator' => 'AND',
      '#conditions' => array(
        array(
          '#name' => 'uc_affiliate2_renewal_status_condition',
          '#title' => t('If the order contains product renewal.'),
          '#argument_map' => array(
            'order' => 'order',
          ),
          '#settings' => array(
          ),
        ),
      ),
    ),
    '#actions' => array(
      array(
        '#name' => 'uc_affiliate2_renewal_email',
        '#title' => t('Send an order email regarding order renewal.'),
        '#argument_map' => array(
          'order' => 'order',
          'recurring_fee' => 'recurring_fee',
        ),
        '#settings' => array(
          'from' => uc_store_email_from(),
          'addresses' => '[order-email]',
          'subject' => uc_get_message('uc_affiliate2_renewal_completed_subject'),
          'message' => uc_get_message('uc_affiliate2_renewal_completed_message'),
          'format' => 1,
        ),
      )
    ),
  );

  return $predicates;
}

/**
 * Implementation of hook_ca_condition().
 */
function uc_affiliate2_ca_condition() {
  $conditions['uc_affiliate2_has_affiliate'] = array(
    '#title' => t('User has affiliate'),
    '#description' => t('Check if the user making the purchase has an affiliate.'),
    '#category' => t('Order: Affiliate2'),
    '#callback' => 'uc_affiliate2_condition_has_affiliate',
    '#arguments' => array(
      'order' => array('#entity' => 'uc_order', '#title' => t('Order')),
    ),
  );
  $conditions['uc_affiliate2_affiliate_level'] = array(
    '#title' => t('Check affiliate level'),
    '#description' => t('Returns TRUE if the current affiliate level is within the parameters below.'),
    '#category' => t('Order: Affiliate2'),
    '#callback' => 'uc_affiliate2_condition_affiliate_level',
    '#arguments' => array(
      'affiliate_commission' => array('#entity' => 'uc_affiliate2_commission', '#title' => t('Affiliate Commission')),
    ),
  );

  return $conditions;
}

/**
 * Implementation of hook_ca_action().
 */
function uc_affiliate2_ca_action() {
  // Send an email to an order with a role expiration
  $actions['uc_affiliate2_set_commission'] = array(
    '#title' => t('Override the commission percentage.'),
    '#category' => t('Affiliate'),
    '#callback' => 'uc_affiliate2_set_commission',
    '#arguments' => array(
      'order' => array(
        '#entity' => 'uc_order',
        '#title' => t('Order'),
      ),
      'affiliate_commission' => array(
        '#entity' => 'uc_affiliate2_commission',
        '#title' => t('Affiliate Commission'),
      ),
    ),
  );

  return $actions;
}

/**
 * Implementation of hook_ca_trigger().
 */
function uc_affiliate2_ca_trigger() {
  $order = array(
    '#entity' => 'uc_order',
    '#title' => t('Order'),
  );
  $affiliate_commission = array(
    '#entity' => 'uc_affiliate2_commission',
    '#title' => t('Affiliate commission'),
  );

  $triggers['uc_affiliate2_apply_commission'] = array(
    '#title' => t('Affiliate commission being applied'),
    '#category' => t('Affiliate2'),
    '#arguments' => array(
      'order' => $order,
      'affiliate_commission' => $affiliate_commission,
    ),
  );
  return $triggers;
}

/**
 * Check if the order contains a renewal product.
 * 
 * @param $order
 *   The order object.
 * @param $recurring_fee
 *   The recurring fee object.
 * @param $settings
 *   The order settings.
 */
function uc_affiliate2_condition_has_affiliate($order, $settings) {
  // check session variable and the database for affiliate for the user making
  // the purchase.
  if ($_SESSION['affiliate'] > 0) {
    return TRUE;
  }
  if (_uc_affiliate2_get_user($uid) > 0) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Check the affiliate level.
 *
 * @see uc_affiliate2_condition_affiliate_level_form()
 */
function uc_affiliate2_condition_affiliate_level($affiliate_commision, $settings) {
  switch ($settings['affiliate_level_comparison']) {
    case 'less':
      return $affiliate_commission->level < $settings['affiliate_level_value'];
    case 'less_equal':
      return $affiliate_commission->level <= $settings['affiliate_level_value'];
    case 'equal':
      return $affiliate_commission->level == $settings['affiliate_level_value'];
    case 'greater_equal':
      return $affiliate_commission->level >= $settings['affiliate_level_value'];
    case 'greater':
      return $affiliate_commission->level > $settings['affiliate_level_value'];
  }
}

/**
 * @see uc_affiliate2_condition_affiliate_level()
 */
function uc_affiliate2_condition_affiliate_level_form($form_state, $settings = array()) {
  $form['affiliate_level_value'] = array(
    '#type' => 'textfield',
    '#title' => t('Affiliate level value'),
    '#description' => t('Specify a value to compare the affiliate level against.'),
    '#default_value' => $settings['affiliate_level_value'],
    '#size' => 16,
  );

  $options = array(
    'less' => t('Total is less than specified value.'),
    'less_equal' => t('Total is less than or equal to specified value.'),
    'equal' => t('Total is equal to specified value.'),
    'greater_equal' => t('Total is greater than or equal to specified value.'),
    'greater' => t('Total is greater than specified value.'),
  );
  $form['affiliate_level_comparison'] = array(
    '#type' => 'radios',
    '#title' => t('Affiliate level comparison type'),
    '#options' => $options,
    '#default_value' => isset($settings['affiliate_level_comparison']) ? $settings['affiliate_level_comparison'] : 'less_equal',
  );

  return $form;
}

/**
 *
 * @see uc_affiliate2_set_commission_form()
 */
function uc_affiliate2_set_commission($order, &$affiliate_commission, $settings) {
  $affiliate_commission->commission = $affiliate_commission->data['total'] * $settings['affiliate_commission_pct'] / 100; 
}

/**
 *
 * @see uc_affiliate2_set_commission()
 */
function uc_affiliate2_set_commission_form($form_state, $settings = array()) {
  $form['affiliate_commission_pct'] = array(
    '#type' => 'textfield',
    '#title' => t('Affiliate Commission Percent'),
    '#description' => t('Enter a value to override the default commission percentage.'),
    '#default_value' => $settings['affiliate_commission_pct']
  );

  return $form;
}
