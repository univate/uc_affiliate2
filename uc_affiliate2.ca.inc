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
  // CA entity for a recurring fee object.
  $entities['uc_affiliate2_commission'] = array(
    '#title' => t('Recurring Fee'),
    '#type' => 'object',
    '#load' => 'uc_affiliate2_fee_user_load',
    '#save' => 'uc_affiliate2_fee_user_save',
  );

  return $entities;
}

/**
 * Implementation of hook_ca_predicate().
 */
function uc_affiliate2_ca_predicate() {
  $predicates = array();

  $predicates['uc_affiliate2_renewal_email_completed'] = array(
    '#title' => t('Notify customer when a renewal has occured.'),
    '#trigger' => 'uc_affiliate2_renewal_complete',
    '#class' => 'notification',
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
    '#description' => t('Check if the user making the purchase as an affiliate.'),
    '#category' => t('Order: Affiliate2'),
    '#callback' => 'uc_affiliate2_condition_has_affiliate',
    '#arguments' => array(
      'order' => array('#entity' => 'uc_order', '#title' => t('Order')),
    ),
  );
  $conditions['uc_affiliate2_renewal_expired_condition'] = array(
    '#title' => t('Check if the recurring fee has expired'),
    '#description' => t('Returns TRUE if the current recurring fee has expired.'),
    '#category' => t('Order: Recurring'),
    '#callback' => 'uc_affiliate2_condition_order_has_expired',
    '#arguments' => array(
      'order' => array('#entity' => 'uc_affiliate2_fee', '#title' => t('Recurring Fee')),
    ),
  );

  return $conditions;
}

/**
 * Implementation of hook_ca_action().
 */
function uc_affiliate2_ca_action() {
  // Send an email to an order with a role expiration
  $actions['uc_affiliate2_renewal_email'] = array(
    '#title' => t('Send an order email regarding order renewal.'),
    '#category' => t('Notification'),
    '#callback' => 'uc_affiliate2_action_renewal_email',
    '#arguments' => array(
      'order' => array(
        '#entity' => 'uc_order',
        '#title' => t('Order'),
      ),
      'recurring_fee' => array(
        '#entity' => 'uc_affiliate2_fee',
        '#title' => t('Recurring Fee'),
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
  $recurring_fee = array(
    '#entity' => 'uc_affiliate2_fee',
    '#title' => t('Recurring Fee'),
  );

  $triggers['uc_affiliate2_renewal_complete'] = array(
    '#title' => t('Recurring order renewal succeeded'),
    '#category' => t('Recurring Fees'),
    '#arguments' => array(
      'order' => $order,
      'recurring_fee' => $recurring_fee,
    ),
  );

  $triggers['uc_affiliate2_renewal_failed'] = array(
    '#title' => t('Recurring order renewal failed'),
    '#category' => t('Recurring Fees'),
    '#arguments' => array(
      'order' => $order,
      'recurring_fee' => $recurring_fee,
    ),
  );

  $triggers['uc_affiliate2_cancel'] = array(
    '#title' => t('Recurring order cancelled'),
    '#category' => t('Recurring Fees'),
    '#arguments' => array(
      'order' => $order,
      'recurring_fee' => $recurring_fee,
    ),
  );

  $triggers['uc_affiliate2_renewal_expired'] = array(
    '#title' => t('Recurring order renewal expired'),
    '#category' => t('Recurring Fees'),
    '#arguments' => array(
      'order' => $order,
      'recurring_fee' => $recurring_fee,
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


