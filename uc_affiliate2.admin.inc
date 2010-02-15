<?php
// $Id: uc_affiliate2.admin.inc,v 1.1.2.1 2010/02/01 06:27:13 univate Exp $


/**
 * @file
 * Admin functions for uc_affiliate2 module.
 *
 */

define('UC_AFFILIATE2_REPORT_DATE_FORMAT', 'M-Y');
define('UC_AFFILIATE2_REPORT_DATE_INTERVAL', '1 month');

/**
 * Define the affiliate users listing
 */
function uc_affiliate2_admin_manage() {
  $user_id = arg(3);
  if ($user_id && is_numeric($user_id)) {
    return uc_affiliate2_admin_users($user_id);
  }

  $clicks = array();
  $users = array();
  $orders = array();

  $result = db_query("SELECT aid, COUNT(click_id) as clicks FROM {uc_affiliate2_clicks} GROUP BY aid");

  while($count = db_fetch_array($result)) {
    $clicks[$count['aid']] = $count['clicks'];
  }

  $result = db_query("SELECT aid, COUNT(aid) as users FROM {uc_affiliate2_users} GROUP BY aid");

  while($count = db_fetch_array($result)) {
    $users[$count['aid']] = $count['users'];
  }

  $result = db_query("SELECT aid, COUNT(aid) as orders FROM {uc_affiliate2_orders} GROUP BY aid");

  while($count = db_fetch_array($result)) {
    $orders[$count['aid']] = $count['orders'];
  }

  // setup the table
  $header = array(
    array('data' => t('Username'), 'field' => 'u.name'),
    array('data' => t('Status'), 'field' => 'u.status'),
    array('data' => t('Member for'), 'field' => 'u.created', 'sort' => 'desc'),
    array('data' => t('Last access'), 'field' => 'u.access'),
    array('data' => t('Unique Clicks')),
    array('data' => t('Users')),
    array('data' => t('Orders')),
  );

  // define the query
  $sql = "SELECT u.uid, u.name, u.status, u.created, u.access
          FROM {users} u
            INNER JOIN {users_roles} ur ON u.uid = ur.uid
          WHERE ur.rid IN (SELECT r.rid
                           FROM {role} r
                             INNER JOIN {permission} p ON r.rid = p.rid
                           WHERE p.perm LIKE '%act as affiliate%')
          GROUP BY u.uid, u.name, u.status, u.created, u.access";

  $sql .= tablesort_sql($header);
  $result = db_query($sql);

  $status = array(t('blocked'), t('active'));

  while ($account = db_fetch_object($result)) {
    $rows[] = array(
      l($account->name, 'admin/store/affiliate/'. $account->uid),
      $status[$account->status],
      format_interval(time() - $account->created),
      $account->access ? t('@time ago', array('@time' => format_interval(time() - $account->access))) : t('never'),
      $clicks[$account->uid] ? $clicks[$account->uid] : '0',
      $users[$account->uid] ? $users[$account->uid] : '0',
      $orders[$account->uid] ? $orders[$account->uid] : '0',
    );
  }

  $output = "<p>". t("Click on an affiliate user to view their monthly sales and commissions.");
  $output .= theme('table', $header, $rows);
  $output .= theme('pager', NULL, 50, 0);

  return $output;
}

function uc_affiliate2_admin_users($uid) {
  // used to determine whether to show the paid form.
  $month_timestamp = arg(4);

  $user = user_load(array('uid' => $uid));

  // setup the table
  $header = array(
    array('data' => t('Month'), 'field' => 'date'),
    array('data' => t('Sales'), 'field' => 'sales'),
    array('data' => t('Commissions'), 'field' => 'commissions'),
    array('data' => t('Paid (click to edit)'), 'field' => 'paid'),
  );

  // define the query
  $sql = 'SELECT ac.commission, uo.order_total, uo.created
          FROM {uc_affiliate2_commission} AS ac
            LEFT OUTER JOIN {uc_orders} AS uo ON ac.order_id = uo.order_id
          WHERE ac.aid = %d';
  $result = db_query($sql, $uid);

  $total_sales = array();
  $total_commissions = array();
  $amounts = _uc_affiliate2_get_amounts('aid', $uid, 'date');

  while ($rec = db_fetch_array($result)) {
    $mondate = date(UC_AFFILIATE2_REPORT_DATE_FORMAT, $rec['created']);
    $total_sales[$mondate] = isset($total_sales[$mondate]) ? $total_sales[$mondate]+$rec['order_total'] : $rec['order_total'];
    $total_commissions[$mondate] = isset($total_commissions[$mondate]) ? $total_commissions[$mondate]+$rec['commission'] : $rec['commission'];
  }

  foreach ($total_sales as $date => $sales) {
    $timestamp = strtotime($date);
    $amount = ($amounts[$timestamp]) ? $amounts[$timestamp] : 0;

    // format paid field as link or form
    if ($month_timestamp && $date == $month_timestamp) {
      $amount = uc_currency_format($amount, false);
      $paid = drupal_get_form('uc_affiliate2_admin_users_paid_form', $amount, $uid, strtotime($date));
    }
    else {
      $amount = uc_currency_format($amount);
      $paid = l($amount, join('/', array(arg(0), arg(1), arg(2), arg(3), $date)));
    }

    // generate record
    $rows[] = array(
      str_replace('-', ' ', $date),
      uc_currency_format($sales),
      uc_currency_format($total_commissions[$date]),
      $paid
    );
  }

  $output = "<p>". t("Displaying affiliate sales and commissions for ") . theme('username', $user) .".";

  $output .= theme('table', $header, $rows);

  return $output;
}

function uc_affiliate2_admin_users_paid_form($form_state, $amount, $uid, $date) {
  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount paid'),
    '#default_value' => $amount,
    '#size' => 8,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $uid
  );
  $form['date'] = array(
    '#type' => 'value',
    '#value' => $date
  );

  $form['#redirect'] = 'admin/store/affiliate/reports/'. date(UC_AFFILIATE2_REPORT_DATE_FORMAT, $date);

  return $form;
}

function uc_affiliate2_admin_users_paid_form_validate($form, &$form_state) {
  $amount = $form_state['values']['amount'];

  if (!is_numeric($amount) || $amount < 0) {
    form_set_error('amount', t('Please specify an amount paid to this affiliate.'));
  }
}

function uc_affiliate2_admin_users_paid_form_submit($form, &$form_state) {
  $sign = variable_get('uc_currency_sign', '$');
  $amount = $form_state['values']['amount'];
  $uid = $form_state['values']['uid'];
  $date = $form_state['values']['date'];

  $rec = db_result(db_query('SELECT amount FROM {uc_affiliate2_paid} WHERE aid=%d AND date=%d', $uid, $date));
  if ($rec===FALSE) {
    db_query('INSERT INTO {uc_affiliate2_paid} (aid, date, amount) VALUES (%d, %d, %f)', $uid, $date, $amount);
  }
  else {
    db_query('UPDATE {uc_affiliate2_paid} SET amount=%f WHERE aid=%d AND date=%d', $amount, $uid, $date);
  }

  $user = user_load(array('uid' => $uid));
  drupal_set_message(t('Registered payment of @amount for !user during @date.', array('@amount' => uc_currency_format($amount), '!user' => theme('username', $user), '@date' => format_date($date, 'custom', 'M Y'))));

  return 'admin/store/affiliate/'. $uid;
}

/**
 * Define the banner/link settings form
 */
function uc_affiliate2_banner_settings() {
  $form['affiliate_banner_template'] = array(
    '#type' => 'textarea',
    '#title' => t('Affiliate Banner Templates'),
    '#description' => t('The IMG tag(s) around which the url of the affiliate is wrapped around. Insert as many as you want.'),
    '#default_value' => variable_get('affiliate_banner_template', ''),
  );
  $form['affiliate_text_link_template'] = array(
    '#type' => 'textfield',
    '#title' => t('Affiliate Text Link Template'),
    '#description' => t('The text around which the url of the affiliate is wrapped around. HTML is allowed.'),
    '#default_value' => variable_get('affiliate_text_link_template', ''),
  );
  $form['affiliate_product_html_template'] = array(
    '#type' => 'textarea',
    '#title' => t('Affiliate Product HTML Template (used for Export)'),
    '#description' => t('When using the html export function in the affiliate panel, this template is used for each product.
                        Allowed placeholders: [LINK], [TITLE], [IMG], [DESCRIPTION], [PRICE].'),
    '#default_value' => variable_get('affiliate_product_html_template', '<div id="prodcontain"><a href="[LINK]"><img src="[IMG]"></a><br /><div id="description">description:</div>[DESCRIPTION]<br /><div id="price">[PRICE]</div></div>'),
  );

  return system_settings_form($form);
}

/**
 * Define the commission settings form
 */
function uc_affiliate2_commission_settings() {
  $form['affiliate_hierarchy_depth'] = array(
    '#type' => 'textfield',
    '#title' => t('Affiliate hierarchy depth'),
    '#description' => t('Controls the depth to which affiliates referrals and commissions are tracked'),
    '#default_value' => variable_get('affiliate_hierarchy_depth', 5),
    '#size' => 3
  );
  $form['affiliate_commission_structure'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Affiliate Commission Structure'),
    '#description' => t('Sets the default commission structure.  This should be a comma separated list of number with the first value corresponding to the first level in the hierarchy and each successive value corresponding to the next level in the hierarchy.  Example: 5,4,2,2,1'),
    '#default_value' => variable_get('affiliate_commission_structure', '5,3,2,2,1'),
    '#size' => 10
  );

  $roles = user_roles(FALSE, 'act as affiliate');

  foreach ($roles as $rid => $role) {
    $form['affiliate_settings_role_'. $rid] = array(
      '#type' => 'fieldset',
      '#title' => t('Settings for @role', array('@role' => $role)),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['affiliate_settings_role_'. $rid]['affiliate_commission_structure_'. $rid] = array(
      '#type' => 'textfield',
      '#title' => t('Affiliate Commission Structure for cookied role: @role', array('@role' => $role)),
      '#description' => t('Sets the commission structure for the role, when the user has returned via cookie. This should be a comma separated list of number with the first value corresponding to the first level in the hierarchy and each successive value corresponding to the next level in the hierarchy.  Example: 5,4,2,2,1'),
      '#default_value' => variable_get('affiliate_commission_structure_'. $rid, '5,3,2,2,1'),
      '#size' => 10
    );
    $form['affiliate_settings_role_'. $rid]['affiliate_commission_'. $rid .'_switch_after'] = array(
      '#type' => 'textfield',
      '#title' => t('Switch to comission settings per role after'),
      '#description' => t('Time after which the per role comission structure settings will be used, instead of the default ones. Strtotime syntax.'),
      '#default_value' => variable_get('affiliate_commission_'. $rid .'_switch_after', '2 days'),
     );
    $form['affiliate_settings_role_'. $rid]['affiliate_set_cookie_'. $rid] = array(
      '#type' => 'checkbox',
      '#title' => t('Set the affiliate cookie for the visiting user.'),
      '#description' => t('By setting the cookie, the affiliate gets credited even when no affiliate id is present in links visited.'),
      '#default_value' => variable_get('affiliate_set_cookie_'. $rid, 0),
    );
    $form['affiliate_settings_role_'. $rid]['affiliate_cookie_lifetime_'. $rid] = array(
      '#type' => 'textfield',
      '#title' => t('Cookie Lifetime'),
      '#description' => t('How long should the cookie be valid. Strtotime syntax.'),
      '#default_value' => variable_get('affiliate_cookie_lifetime_'. $rid, '1 year'),
    );
  }

  return system_settings_form($form);
}

/**
 * Define the affiliate admin settings form
 */
function uc_affiliate2_admin_settings() {
  foreach (uc_order_status_list('general') as $status) {
    $statuses[$status['id']] = $status['title'];
  }
  $form['affiliate_commission_order_statuses'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Order status'),
    '#default_value' => variable_get('affiliate_commission_order_statuses', array('completed' => 'completed')),
    '#description' => t('Where in the order status(es) will the commissions be assigned.'),
    '#options' => $statuses,
  );
  $form['affiliate_invalid_redirect'] = array(
    '#type' => 'textfield',
    '#title' => t('On an invalid affiliate URL, redirect to'),
    '#description' => t('When an invalid username is used as a subdomain, the user will be redirected to this URL.'),
    '#default_value' => variable_get('affiliate_invalid_redirect', '')
  );
  $form['affiliate_enable_subdomain_handling'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable affiliate subdomain handling'),
    '#description' => t("If enabled, this module will record affiliate actions if customers visit the site through an affiliate subdomain, like john.thissite.com. Enable this only if drupal is not installed on a subdomain."),
    '#default_value' => variable_get('affiliate_enable_subdomain_handling', 0),
  );
  $form['affiliate_enable_recurring_commission'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable affiliate commissions for subsequent orders'),
    '#description' => t("If enabled, on subsequent orders after a user has made a purchase will grant commissions to the original affiliate if no affiliate information is available from the session or cookie."),
    '#default_value' => variable_get('affiliate_enable_recurring_commission', 0),
  );

  foreach (_payment_method_list() as $method) {
    $payment_methods[$method['id']] = $method['name'];
  }

  $form['affiliate_no_commission_payment_methods'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Disable commission for these Payment methods'),
    '#default_value' => variable_get('affiliate_no_commission_payment_methods', array()),
    '#description' => t('You can disable commission for certain Payment methods'),
    '#options' => $payment_methods,
  );

  return system_settings_form($form);
}

function uc_affiliate2_admin_settings_validate($form, &$form_state) {
  $url = $form_state['values']['affiliate_invalid_redirect'];

  if (!empty($url) && ($url != check_url($url) || strpos($url, '://') !== FALSE)) {
    form_set_error('affiliate_invalid_redirect', t('Please enter a valid Drupal path, without the http and domain parts.'));
  }

  $roles = user_roles(FALSE, 'act as affiliate');
  foreach ($roles as $rid => $role) {
    if (!strtotime($form_state['values']['affiliate_cookie_lifetime_'. $rid])) {
      form_set_error('affiliate_cookie_lifetime_'. $rid, t('Invalid strtotime syntax.'));
    }
  }
}

/**
 * Build a monthly report.
 */
function uc_affiliate2_monthly_report() {
   // The month we are viewing. If this is empty, the current month is assumed.
  $month = arg(4);

  $date_start = strtotime($month);
  if ($date_start === FALSE || $date_start == -1) { // PHP > 5.1.0 returns FALSE instead of -1
    $month = date(UC_AFFILIATE2_REPORT_DATE_FORMAT);
    $date_start = strtotime($month);
  }
  $date_end = strtotime('+'. UC_AFFILIATE2_REPORT_DATE_INTERVAL, $date_start);


   // Used to determine whether to show the paid form
  $uid_external = arg(5);

  // setup the table
  $header = array(
    array('data' => t('Affiliate'), 'field' => 'aid'),
    array('data' => t('Sales'), 'field' => 'sales'),
    array('data' => t('Commissions'), 'field' => 'commissions'),
    array('data' => t('Paid (click to edit)'), 'field' => 'paid'),
  );

  // define the query
  $sql = 'SELECT ac.aid, ac.commission, uo.order_total FROM {uc_affiliate2_commission} AS ac LEFT OUTER JOIN {uc_orders} AS uo ON ac.order_id = uo.order_id WHERE uo.created BETWEEN %d AND %d';
  $result = db_query($sql, $date_start, $date_end);

  $total_sales = array();
  $total_commissions = array();
  $amounts = _uc_affiliate2_get_amounts('date', $date_start, 'aid');

  while ($rec = db_fetch_array($result)) {
    $aid = $rec['aid'];
    $total_sales[$aid] = isset($total_sales[$aid]) ? $total_sales[$aid] + $rec['order_total'] : $rec['order_total'];
    $total_commissions[$aid] = isset($total_commissions[$aid]) ? $total_commissions[$aid] + $rec['commission'] : $rec['commission'];
  }

  $user_results = db_query('SELECT name, uid FROM {users}');
  $users = array();

  while ($user = db_fetch_array($user_results)) {
    $users[$user['uid']] = $user['name'];
  }

  foreach ($total_sales as $uid => $sales) {
    $amount = ($amounts[$uid]) ? $amounts[$uid] : 0;
    $amount = uc_currency_format($amount);

    // format paid field as link or form
    if ($uid_external && is_numeric($uid_external) && $uid == $uid_external) {
      $paid = drupal_get_form('uc_affiliate2_admin_users_paid_form', $amount, $uid, $date_start);
    }
    else {
      $paid = l($amount, join('/', array(arg(0), arg(1), arg(2), arg(3), $month, $uid)));
    }

    $rows[] = array(
      $users[$uid],
      uc_currency_format($sales),
      uc_currency_format($total_commissions[$uid]),
      $paid
    );
  }

  $output = "<p>". t("Displaying affiliate sales and commissions for ") . str_replace('-', ' ', $month) .". ";
  $output .= t("Show ") . l(t('previous month'), join('/', array(arg(0), arg(1), arg(2), arg(3), date(UC_AFFILIATE2_REPORT_DATE_FORMAT, strtotime('-'. UC_AFFILIATE2_REPORT_DATE_INTERVAL, $date_start)))));
  $output .= t(" or ") . l(t('next month'), join('/', array(arg(0), arg(1), arg(2), arg(3), date(UC_AFFILIATE2_REPORT_DATE_FORMAT, $date_end)))) .'.</p>';

  $output .= theme('table', $header, $rows);

  return $output;
}

/**
 *
 */
function uc_affiliate2_order_commissions($order_id) {
  $result = db_query('SELECT ac.commission_id, ac.aid, u.name, ac.commission, ac.commission_notes
                      FROM {uc_affiliate2_commission} ac
                        LEFT JOIN {users} as u ON u.uid=ac.aid
                      WHERE ac.order_id=%d', $order_id);

  $context = array(
    'revision' => 'themed',
    'type' => 'product',
  );

  $rows = array();
  while ($commission = db_fetch_object($result)) {
    $rows[] = array(
      l($commission->name, 'user/'. $commission->aid),
      uc_price($commission->commission, $context),
      $commission->commission_notes,
      l('edit', 'admin/store/orders/'. $order_id .'/affiliates/'. $commission->commission_id .'/edit', array('attributes' => array('class' => 'edit'))) .' | '.
      l('remove', 'admin/store/orders/'. $order_id .'/affiliates/'. $commission->commission_id .'/remove')
    );
  }
  return theme('table', array('Affiliate', 'Commission', 'Notes', ''), $rows);
}

/**
 * Manually edit a commission on an order.
 */
function uc_affiliate2_order_edit_commissions($form_state, $order_id, $commission_id) {
  $commission = db_fetch_object(db_query("SELECT commission_id, aid, commission, commission_notes FROM {uc_affiliate2_commission} WHERE commission_id=%d", $commission_id));

  $form['commission_id'] = array(
    '#type' => 'hidden',
    '#value' => $commission->commission_id
  );
  $form['order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id
  );
  $form['commission'] = array(
    '#type' => 'textfield',
    '#title' => t('Commission Amount'),
    '#default_value' => $commission->commission,
  );
  $form['commission_notes'] = array(
    '#type' => 'textfield',
    '#title' => t('Commission Note'),
    '#default_value' => $commission->commission_notes,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update commission'),
    '#suffix' => l(t('Cancel'), uc_referer_uri()),
  );
  return $form;
}

function uc_affiliate2_order_edit_commissions_submit($form, &$form_state) {
  $commission = db_fetch_object(db_query("SELECT * FROM {uc_affiliate2_commission} WHERE commission_id=%d", $form_state['values']['commission_id']));
  $commission->commission = $form_state['values']['commission'];
  $commission->commission_notes = $form_state['values']['commission_notes'];
  uc_affiliate2_save_commission($commission);
  $form_state['redirect'] = 'admin/store/orders/'. $form_state['values']['order_id'] .'/affiliates';
}

/**
 * Delete a commission from an order.
 */
function uc_affiliate2_order_remove_commissions($form_state, $order_id, $commission_id) {
  $form['commission_id'] = array(
    '#type' => 'hidden',
    '#value' => $commission_id
  );
  $form['order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Remove commission'),
    '#suffix' => l(t('Cancel'), uc_referer_uri()),
  );


  return $form;
}

function uc_affiliate2_order_remove_commissions_submit($form, &$form_state) {
  db_query('DELETE FROM {uc_affiliate2_commission} WHERE commission_id=%d', $form_state['values']['commission_id']);
  $form_state['redirect'] = 'admin/store/orders/'. $form_state['values']['order_id'] .'/affiliates';
}
