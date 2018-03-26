package net.nestal.heartyrabbit;

/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.InputStreamReader;
import java.net.CookieManager;
import java.net.HttpCookie;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;

/**
 * Created by nestal on 3/26/18.
 */

public class HeartyRabbit
{
	private String m_site;
	private CookieManager m_cookies;

	public HeartyRabbit(String site)
	{
		// per-VM cookie manager
		m_cookies = new CookieManager();
		CookieManager.setDefault(m_cookies);

		m_site = site;
	}

	public boolean login(String username, String password) throws Exception
	{
		URL login_url = new URL("https://" + m_site + "/login");
		HttpsURLConnection conn = (HttpsURLConnection)login_url.openConnection();
		conn.setDoOutput(true);
		conn.setDoInput(true);

		conn.setRequestMethod("POST");
		conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
		conn.setInstanceFollowRedirects(false);

		byte[] data = ("username=" + username + "&password=" + password).getBytes(
			StandardCharsets.UTF_8);
		conn.setRequestProperty("Content-Length", Integer.toString(data.length));

		conn.getDoOutput();
		try( DataOutputStream wr = new DataOutputStream( conn.getOutputStream()))
		{
			wr.write(data);
		}

		Log.i("Login", "response code: " + conn.getResponseCode());

		BufferedReader in = new BufferedReader(
			new InputStreamReader(conn.getInputStream()));
		String inputLine;
		StringBuffer response = new StringBuffer();

		while ((inputLine = in.readLine()) != null)
		{
			response.append(inputLine);
		}
		Log.i("Login", "response content: " + response.toString());

		// Apply the cookie to the manager
		List<String> cookie_header = conn.getHeaderFields().get("Set-Cookie");
		if (cookie_header != null)
		{
			for (String cookie_str : cookie_header)
			{
				HttpCookie cookie = HttpCookie.parse(cookie_str).get(0);

				if ("id".equals(cookie.getName()) && !cookie.getValue().isEmpty())
				{
					Log.w("login", "cookie = " + cookie.getName() + ": " + cookie.getValue());
					m_cookies.getCookieStore().add(null, HttpCookie.parse(cookie_str).get(0));
					return true;
				}
			}
		}

		return false;
	}

	public void upload(ArrayList<Uri> images)
	{

	}
}
