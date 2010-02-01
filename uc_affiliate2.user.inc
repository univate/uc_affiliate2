<?php
// $Id: uc_affiliate2.user.inc,v 1.1.2.1 2010/02/01 06:27:13 univate Exp $


/**
 * @file
 * User functions for the uc_affiliate2 module.
 *
 */

/*
 * Display dashboard for affiliate
 */
function uc_affiliate2_dashboard($user) {
  $commissions = array('today' => 0, 'week' => 0, 'year' => 0);
  $clicks = array('today' => 0, 'week' => 0, 'year' => 0);

  // get today's commissions
  $dates_today = _uc_affiliate2_predefined_range(7);

  $sql = 'SELECT COUNT(ac.commission_id) as cnt FROM {uc_affiliate2_commission} ac INNER JOIN {uc_orders} uco on ac.order_id = uco.order_id WHERE aid = %d AND uco.created BETWEEN %d AND %d';
  $result = db_query($sql, $user->uid, $dates_today['start'], $dates_today['end']);

  while ($count = db_fetch_array($result)) {
    $commissions['today'] += $count['cnt'];
  }

  /// get commissions from the last 7 days
  $dates_week = _uc_affiliate2_predefined_range(6);

  $sql = 'SELECT COUNT(ac.commission_id) as cnt
            FROM {uc_affiliate2_commission} ac
              INNER JOIN {uc_orders} uco on ac.order_id = uco.order_id
                WHERE aid = %d AND uco.created BETWEEN %d AND %d';
  $result = db_query($sql, $user->uid, $dates_week['start'], $dates_week['end']);

  while ($count = db_fetch_array($result)) {
    $commissions['week'] += $count['cnt'];
  }

  // get commissions from the last year
  $dates_year = _uc_affiliate2_predefined_range(5);

  $sql = 'SELECT COUNT(ac.commission_id) as cnt
            FROM {uc_affiliate2_commission} ac
              INNER JOIN {uc_orders} uco on ac.order_id = uco.order_id
                WHERE aid = %d AND uco.created BETWEEN %d AND %d';
  $result = db_query($sql, $user->uid, $dates_year['start'], $dates_year['end']);

  while ($count = db_fetch_array($result)) {
    $commissions['year'] += $count['cnt'];
  }

   // get today's clicks
  $sql = "SELECT count(click_id) as clicks from {uc_affiliate2_clicks}
            WHERE aid = %d AND time BETWEEN %d AND %d
              GROUP BY aid";
  $result = db_query($sql, $user->uid, $dates_today['start'], $dates_today['end']);

  while ($count = db_fetch_array($result)) {
    $clicks['today'] += $count['clicks'];
  }

  // get commissions from the last 7 days
  $sql = "SELECT count(click_id) as clicks from {uc_affiliate2_clicks}
            WHERE aid = %d AND time BETWEEN %d AND %d
              GROUP BY aid";
  $result = db_query($sql, $user->uid, $dates_week['start'], $dates_week['end']);

  while ($count = db_fetch_array($result)) {
    $clicks['week'] += $count['clicks'];
  }

  // get commissions from the last year
  $sql = "SELECT count(click_id) as clicks from {uc_affiliate2_clicks}
            WHERE aid = %d AND time BETWEEN %d AND %d
              GROUP BY aid";
  $result = db_query($sql, $user->uid, $dates_year['start'], $dates_year['end']);

  while ($count = db_fetch_array($result)) {
    $clicks['year'] += $count['clicks'];
  }

  drupal_set_title(t('Affiliate Center'));

  $output = '';

  $output .= '<div id="affiliate-sales-header"><h2>' . t('Commission History') . '</h2> <br />';

  $output .=  t('Sales Today:') . " $commissions[today] <br />" .
              t('Sales last 7 Days:') . " $commissions[week] <br />" .
              t('Last 365 Days:') . " $commissions[year] <br /><br />";

  $commissions_url = join('/', array('user', $user->uid, 'affiliate', 'commissions'));
  $output .= l(t('Click here for more reports.'), $commissions_url, array('query' => NULL, 'fragment' => NULL, 'absolute' => TRUE, 'html' => TRUE));

  $output .= '</div>';

  $output .= '<br /><div id="affiliate-sales-click-count"><h2>' . t('Unique Click Count') . '</h2> <br />';

  $output .= t("Today's Clicks:") . " $clicks[today] <br />" .
             t("Last 7 Days: ") . " $clicks[week] <br />" .
             t("Last 365 Days: ") . " $clicks[year] <br /><br />";

  $clicks_url = join('/', array('user', $user->uid, 'affiliate', 'clicks'));
  $output .= l(t('Click here for customized click reporting.'), $clicks_url, array('query' => NULL, 'fragment' => NULL, 'absolute' => TRUE, 'html' => TRUE));

  $output .= '</div>';

  $output .= '<br /><div id="affiliate-links-header"><h2>' . t('Affiliate Links') . '</h2>';
  $output .= t('You can append a destination page to the end of the affiliate links to redirect to a specific page,
              as well as track clicks to it, i.e.
              example.com/!path/!uid/node/1', array('!uid' => $user->uid, '!path' => variable_get('uc_affiliate2_path', 'affiliate'))) . '<br /> <br />';

  $text_link = variable_get('affiliate_text_link_template', '');
  $text_link = l($text_link, variable_get('uc_affiliate2_path', 'affiliate'). '/'. $user->uid, array('query' => NULL, 'fragment' => NULL, 'absolute' => TRUE, 'html' => TRUE));
  $output .= '<b>' . t('Text Link:') . '</b> <br /> <textarea cols="45">'. $text_link .'</textarea>';

  $output .= _uc_affiliate2_display_banners();

  $links_url = join('/', array('user', $user->uid, 'affiliate', 'links'));
  $output .= '<br />' . l(t('Click here for product links.'), $links_url, array('query' => NULL, 'fragment' => NULL, 'absolute' => TRUE, 'html' => TRUE));

  $output .= '</div>';

  return $output;
}

function uc_affiliate2_links($user) {
  $output = '';

  $products = array('all' => t('Show All'));
  $products += _uc_affiliate2_get_products();

  if (count($products) > 1) {
    $output .= '<b>' . t('Export product feed') . '</b> <br />';
    $output .= t('Choose the products which you want included in your feed.') . '<br />';
    $output .= t('All product links will have your affiliate id included.') . '<br /><br /> ';

    $output .= drupal_get_form('_uc_affiliate2_product_form', $products, $user);
  }
  else {
    $output .= t('No products found.');
  }

  return $output;
}

// This function exports the products in html
function uc_affiliate2_links_export() {
  $ids = func_get_args();

  $output = '<br />';
  $output .= t('!image is the placeholder image in case none is present.',
                  array('!image' => '<img src="'. $GLOBALS['base_url'] .'/misc/watchdog-error.png'.'" />'));
  $output .= '<br />';

  if ($ids[0] == 'all') {
    $products = _uc_affiliate2_get_products('html');
  }
  else {
    $products = _uc_affiliate2_get_products('html', $ids);
  }

  $output .= _uc_affiliate2_display_products($products);

  return $output;
}

// Show the html products in a table
function _uc_affiliate2_display_products($products) {
  global $base_url;

  $header[] = array('field' => 'product_title', 'data' => t('Product Title'));
  $header[] = array('field' => 'preview', 'data' => t('Preview'));
  $header[] = array('field' => 'code', 'data' => t('Code'));

  if(count($products) < 1) {
    return t('No products found.');
  }

  foreach ($products as $key => $product) {
    $line = array();

    $line[] = $key;
    $line[] = $product;
    $line[] = '<textarea cols="60" rows="4">'. $product .'</textarea>';

    $rows[] = $line;
  }

  return theme('table', $header, $rows);;
}

function _uc_affiliate2_product_form(&$form_state, $products, $user) {
  drupal_add_js(drupal_get_path('module', 'uc_affiliate2') .'/uc_affiliate2.js');

  $form['uid'] = array('#type' => 'value', '#value' => $user->uid);

  $form['products'] = array(
    '#type' => 'checkboxes',
    '#columns' => 2,
    '#options' => $products,
    '#theme' => 'uc_affiliate2_columns_checkboxes',
  );

  $form['export_type'] = array(
    '#type' => 'radios',
    '#title' => t('Export in'),
    '#options' => array('html' => 'HTML', 'csv' => 'CSV'),
    '#default_value' => 'csv',
  );

  $form['submit'] = array('#type' => 'submit', '#value' => t('Export'));

  return $form;
}

function _uc_affiliate2_product_form_submit($form, &$form_state) {
  global $user;

  $show_these = array();

  // build the filter array for _uc_affiliate2_get_products from the form checkboxes
  foreach ($form_state['values']['products'] as $product_id => $status) {
    if ($status !== 0) {
      $show_these[] = $product_id;
    }
  }

  if (in_array('all', $show_these)) {
      $show_these = array('all'); // we don't need anything else
  }

  if (count($show_these) > 0) {
    if ($form_state['values']['export_type'] == 'csv') {
      drupal_set_header("Content-Type: application/csv; charset=\"utf-8\"\n");
      drupal_set_header('Content-Disposition: attachment; filename="export.csv"');

      if ($show_these[0] == 'all') {
        print _uc_affiliate2_get_products('csv');
      }
      else {
        print _uc_affiliate2_get_products('csv', $show_these);
      }

      //module_invoke_all('exit'); - this causes a header warning in the watchlog because the form tries to redirect to itself
      exit;
    }
    else { // html
      $ids = join('/', $show_these);
      $path = join('/', array('user', $form_state['values']['uid'], 'affiliate', 'links', 'export', $ids));

      drupal_goto($path);
    }
  }
  else {
    drupal_set_message(t('You must select at least one product.'));
  }
}

function _uc_affiliate2_display_banners() {
  global $base_url, $user;

  $banners = variable_get('affiliate_banner_template', '');

  $header[] = array('field' => 'banner_preview', 'data' => t('Banner Preview'));
  $header[] = array('field' => 'banner_code', 'data' => t('Banner Code'));

  $banners = explode('<', $banners);
  unset($banners[0]);

  if(count($banners) < 1) {
    return;
  }

  foreach ($banners as $banner) {
    $banner = '<'. $banner;
    $banner_link = l($banner, variable_get('uc_affiliate2_path', 'affiliate') .'/'. $user->uid, array('query' => NULL, 'fragment' => NULL, 'absolute' => TRUE, 'html' => TRUE));

    $line = array();
    $line[] = $banner;
    $line[] = '<textarea cols="45">'. $banner_link .'</textarea>';
    $rows[] = $line;
  }

  return theme('table', $header, $rows);
}

/*
 * Display affiliate commission records
 */
function uc_affiliate2_commissions($user) {
  drupal_set_title(t('Commissions'));

  // setup the table
  $header = array(
    array('data' => t('Customer')),
    array('data' => t('Order'), 'field' => 'order_id', 'sort' => 'desc'),
    array('data' => t('Date'), 'field' => 'order_date'),
    array('data' => t('Notes')),
    array('data' => t('Purchase'), 'field' => 'order_total'),
    array('data' => t('Commission'), 'field' => "commission"),
  );

  // Create sql to affiliate's commissions
  $dates = _uc_affiliate2_predefined_range(arg(4));
  $sql = 'SELECT uco.uid, uco.billing_first_name, uco.billing_last_name, ac.order_id, uco.created as order_date, ac.commission_notes, uco.order_total, ac.commission
            FROM {uc_affiliate2_commission} ac
              INNER JOIN {uc_orders} uco on ac.order_id = uco.order_id
                WHERE aid = %d AND uco.created BETWEEN %d AND %d';
  $sql .= tablesort_sql($header);
  $result = db_query($sql, $user->uid, $dates['start'], $dates['end']);

  $total_sales = 0;
  $total_commission = 0;
  while ($commission = db_fetch_array($result)) {
    $u = l($commission['billing_first_name'] .' '. $commission['billing_last_name'], 'user/'. $commission['uid']);
    $rows[] = array(
      $u,
      $commission['order_id'],
      format_date($commission['order_date'], 'custom', 'Y/m/d'),
      $commission['commission_notes'],
      uc_currency_format($commission['order_total']),
      uc_currency_format($commission['commission'])
    );
    $total_sales += $commission['order_total'];
    $total_commmission += $commission['commission'];
  }
  $rows[] = array('', '', '', t('Total:'), uc_currency_format($total_sales), uc_currency_format($total_commmission));

  $output = theme('table', $header, $rows);
  $output .= drupal_get_form('uc_affiliate2_time_selection_form');

  return $output;
}

/*
 * Display affiliate clicks
 */
function uc_affiliate2_clicks($user) {
  drupal_set_title(t('Unique Click count'));

  // Create sql to display level of affiliate specified
  $dates = _uc_affiliate2_predefined_range(arg(4));
  $sql = "SELECT time FROM {uc_affiliate2_clicks} WHERE aid = %d AND time BETWEEN %d AND %d";
  $result = db_query($sql, $user->uid, $dates['start'], $dates['end']);

  $clicks = array();
  while ($click_day = db_fetch_array($result)) {
    $click_day['time'] = (int)format_date($click_day['time'], custom, 'U');

    $date = date('Y/m/d', $click_day['time']);
    $date_unix = strtotime($date);

    $clicks[$date_unix] += 1;
  }

  /*
   * Since we are building the array by hand instad of grouping with date functions
   * (because mysql/postgresql have different ways to do this, and we need to support both),
   * we need to simulate the sorting
   */
  $header = array(
    array('data' => t('Date'), 'field' => "date"),
    array('data' => t('Unique Clicks'), 'field' => "clicks"),
  );

  $sort = str_replace('ORDER BY', '', tablesort_sql($header));
  $sort_params = explode(' ', trim($sort));

  if ($sort_params[0] == 'date' && $sort_params[1] == 'ASC') {
    ksort($clicks);
  } else if ($sort_params[0] == 'date' && $sort_params[1] == 'DESC') {
    krsort($clicks);
  } else if ($sort_params[0] == 'clicks' && $sort_params[1] == 'ASC') {
    asort($clicks);
  } else if ($sort_params[0] == 'clicks' && $sort_params[1] == 'DESC') {
    arsort($clicks);
  }

  $total_clicks = 0;
  foreach($clicks as $click_date => $clicks_count) {
    $click_date_end = $click_date + 86399;
    $link = l(date('Y/m/d', $click_date), 'user/' . $user->uid . '/affiliate/clicks/details/' . $click_date . '/' . $click_date_end);
    $rows[] = array($link, $clicks_count);
    $total_clicks += $clicks_count;
  }
  $rows[] = array(t('Total:'), $total_clicks);

  // Build the output
  global $pager_total_items;

  $output = theme('table', $header, $rows);
  $output .= drupal_get_form('uc_affiliate2_time_selection_form');

  return $output;
}

/*
 * Display a detailed clicks view for the selected date
 */
function uc_affiliate2_click_details($user) {
  drupal_set_title(t('Unique Clicks - Details'));

  $cookie_statuses = array(t('Not sent'), t('Sent (Unconfirmed)'), t('Sent (Confirmed)'));

  $output = t('Viewing clicks for: ') . date('Y/m/d', arg(5)) . '<br /> <br />';
  $output .= t('Cookie Status can be: Not sent, Sent (Unconfirmed), Sent (Confirmed).') . '<br />';

  // setup the table
  $header = array(
    array('data' => t('Date/Time'), 'field' => "time", 'sort' => 'desc'),
    array('data' => t('Client IP'), 'field' => "client_ip"),
    array('data' => t('Referrer'), 'field' => "referrer"),
    array('data' => t('Destination'), 'field' => "destination"),
    array('data' => t('Cookie Status'), 'field' => "cookie_status"),
  );

  // Create sql to display level of affiliate specified
  $dates = _uc_affiliate2_predefined_range(0);

  $sql = "SELECT * FROM {uc_affiliate2_clicks} WHERE aid = %d AND time BETWEEN %d AND %d";

  $sql .= tablesort_sql($header);
  $result = db_query($sql, $user->uid, $dates['start'], $dates['end']);

  $total_clicks = 0;
  while ($click = db_fetch_array($result)) {
    if ($click['referrer']) {
      $click['referrer'] = l(t('Referrer'), $click['referrer']); // make it a link
    } else {
      $click['referrer'] = 'N/A';
    }

    if ($click['destination']) {
      $click['destination'] = l(t('Destination'), $click['destination']); // make it a link
    } else {
      $click['destination'] = 'Frontpage';
    }

    $rows[] = array(format_date($click['time'], 'custom', 'Y/m/d H:i:s'),
                    $click['client_ip'] ? $click['client_ip'] : 'N/A',
                    $click['referrer'],
                    $click['destination'],
                    ($click['cookie_status'] === NULL) ? 'Not specified' : $cookie_statuses[$click['cookie_status']]
                    );

    $total_clicks += 1;
  }
  $rows[] = array(t('Total:'), $total_clicks);

  // Build the output
  global $pager_total_items;

  $output .= theme('table', $header, $rows);

  return $output;
}

/*
 * Display affiliate orders by status
 */
function uc_affiliate2_orders($user) {
  drupal_set_title(t('All Orders'));

  // setup the table
  $header = array(
    array('data' => t('Customer')),
    array('data' => t('Order'), 'field' => 'ao.order_id', 'sort' => 'desc'),
    array('data' => t('Order Status'), 'field' => 'order_status'),
    array('data' => t('Date'), 'field' => 'created'),
    array('data' => t('Purchase'), 'field' => 'order_total'),
  );

  // Create sql to affiliate's commissions
  $sql = 'SELECT uco.*, ao.order_id
            FROM {uc_affiliate2_orders} ao INNER JOIN {uc_orders} uco on ao.order_id = uco.order_id
              WHERE ao.aid = %d';
  $sql .= tablesort_sql($header);
  $result = db_query($sql, $user->uid);

  $total_sales = 0;
  while ($order = db_fetch_array($result)) {
    if($order['uid'] > 0) {
      $u = l($order['billing_first_name'] .' '. $order['billing_last_name'], 'user/'. $order['uid']);
    } else {
      $u = $order['billing_first_name'] .' '. $order['billing_last_name'];
    }

    $rows[] = array(
      $u,
      $order['order_id'],
      ucwords(str_replace('_', ' ', $order['order_status'])),
      format_date($order['created'], 'custom', 'm-d-Y'),
      uc_currency_format($order['order_total']),
    );
    $total_sales += $order['order_total'];
  }
  $rows[] = array('', '', '', t('Total:'), uc_currency_format($total_sales));

  $output = theme('table', $header, $rows);

  return $output;
}

/*
 * Display affiliate's downline
 */
function uc_affiliate2_downline($user, $level = 1) {
  drupal_set_title(t('Downline'));

  //make sure user doesn't try to manipulate depth
  if ($level > variable_get('affiliate_hierarchy_depth', 5)) {
    $level = variable_get('affiliate_hierarchy_depth', 5);
  }

  // setup the table
  $header = array(
    array('data' => t('Username'), 'field' => "u$level.name"),
    array('data' => t('Affiliate'), 'field' => "u$level.status"),
    array('data' => t('Member for'), 'field' => "u$level.created", 'sort' => 'desc'),
    array('data' => t('Last access'), 'field' => "u$level.access"),
    array('data' => t('Level')),
    array('data' => t('Customers'), 'field' => "uc.user_count"),
  );

  // Create sql to display level of affiliate specified
  $sql = "SELECT u1.uid, uc.user_count ";
  $sql .= "FROM {users} u1 ";
  for ($i=1; $i<=$level; $i++) {
    $next = $i+1;
    $sql .= "INNER JOIN {uc_affiliate2_users} au$i ON au$i.uid = u$i.uid ";
    $sql .= "INNER JOIN {users} u$next ON au$i.aid = u$next.uid ";
  }
  $sql .= "LEFT OUTER JOIN (SELECT au.aid, COUNT(au.uid) as user_count FROM {uc_affiliate2_users} au GROUP BY au.aid) uc on uc.aid = u1.uid ";
  $sql .= "WHERE u$next.uid = ". $user->uid;

  $sql .= tablesort_sql($header);
  $result = pager_query($sql, 25);

  // create the data rows for our table
  while ($affiliate = db_fetch_object($result)) {
    $downline_user = user_load(array('uid' => $affiliate->uid));
    $rows[] = array(
      theme('username', $downline_user),
      user_access('act as affiliate', $downline_user) ? 'Yes' : 'No',
      format_interval(time() - $downline_user->created),
      $downline_user->access ? t('%time ago', array('%time' => format_interval(time() - $downline_user->access))) : t('never'),
      $level,
      $affiliate->user_count ? $affiliate->user_count : 0
    );
  }

  // Build the output
  $output = '<ul class="tabs secondary">';
  for ($i=1; $i<=variable_get('affiliate_hierarchy_depth', 5); $i++) {
    $output .= ($i == $level ? '<li class="active">' : '<li>');
    $output .= l(t("Level") ." ". $i, 'user/'. $user->uid .'/affiliate/downline/'. $i) ."</li>";
  }
  $output .= "</ul>";

  global $pager_total_items;
  $output .= '<h2>'. t('Total Level !level, Customers: !customers', array('!level' => $level, '!customers' => $pager_total_items[0])) .'</h2>';
  $output .= theme('table', $header, $rows);
  $output .= theme('pager', NULL, 50, 0);
  return $output;
}

/*
 * Display affiliate's upline
 */
function uc_affiliate2_upline($user) {
  drupal_set_title(t('Upline'));

  // setup the table
  $header = array(
    array('data' => t('Level')),
    array('data' => t('Username')),
    array('data' => t('Member for')),
    array('data' => t('Last access')),
    array('data' => t('Customers')),
  );

  // Create sql to display user's upline
  $level = variable_get('affiliate_hierarchy_depth', 5);
  $rows = array();
  $next_uid = $user->uid;
  for ($i=1; $i<=$level; $i++) {
    $sql = "SELECT u.uid, u.name, u.status, u.created, u.access, uc.user_count, ". $i ." AS level ";
    $sql .= "FROM {users} u INNER JOIN {uc_affiliate2_users} au ON au.aid = u.uid ";
    $sql .= "LEFT OUTER JOIN (SELECT au2.aid, COUNT(au2.uid) as user_count FROM {uc_affiliate2_users} au2 GROUP BY au2.aid) uc on uc.aid = u.uid ";
    $sql .= "WHERE au.uid = %d";

    // create the data rows for our table
    $result = db_query($sql, $next_uid);
    $status = array(t('blocked'), t('active'));
    while ($affiliate = db_fetch_object($result)) {
      $next_uid = $affiliate->uid;
      $rows[] = array(
        $affiliate->level,
        theme('username', $affiliate),
        format_interval(time() - $affiliate->created),
        $affiliate->access ? t('%time ago', array('%time' => format_interval(time() - $user->access))) : t('never'),
        $affiliate->user_count ? $affiliate->user_count : 0
      );
    }
  }

  if (count($rows) > 0) {
    $output = theme('table', $header, $rows);
  }
  else {
    $output = "<p>". t("You are a top level affiliate.  There are no affiliates in your upline.") ."</p>";
  }
  return $output;
}


