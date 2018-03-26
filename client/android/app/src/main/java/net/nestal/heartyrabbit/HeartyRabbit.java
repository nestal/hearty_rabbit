package net.nestal.heartyrabbit;

/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.franmontiel.persistentcookiejar.ClearableCookieJar;
import com.franmontiel.persistentcookiejar.PersistentCookieJar;
import com.franmontiel.persistentcookiejar.cache.SetCookieCache;
import com.franmontiel.persistentcookiejar.persistence.SharedPrefsCookiePersistor;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.CookieManager;
import java.net.CookiePolicy;
import java.net.HttpCookie;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.cert.CertificateException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import okhttp3.Cookie;
import okhttp3.FormBody;
import okhttp3.HttpUrl;
import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

/**
 * Created by nestal on 3/26/18.
 */

public class HeartyRabbit
{
	private String m_site;
	private String m_user;
	private ContentResolver m_resolver;
	private ClearableCookieJar m_cookieJar;
	private OkHttpClient m_client ;

	private static OkHttpClient getUnsafeOkHttpClient(ClearableCookieJar jar) {
		try {
			// Create a trust manager that does not validate certificate chains
			final TrustManager[] trustAllCerts = new TrustManager[] {
				new X509TrustManager() {
					@Override
					public void checkClientTrusted(java.security.cert.X509Certificate[] chain, String authType) throws
						CertificateException
					{
					}

					@Override
					public void checkServerTrusted(java.security.cert.X509Certificate[] chain, String authType) throws CertificateException {
					}

					@Override
					public java.security.cert.X509Certificate[] getAcceptedIssuers() {
						return new java.security.cert.X509Certificate[]{};
					}
				}
			};

			// Install the all-trusting trust manager
			final SSLContext sslContext = SSLContext.getInstance("SSL");
			sslContext.init(null, trustAllCerts, new java.security.SecureRandom());
			// Create an ssl socket factory with our all-trusting manager
			final SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();

			OkHttpClient.Builder builder = new OkHttpClient.Builder();
			builder.sslSocketFactory(sslSocketFactory, (X509TrustManager)trustAllCerts[0]);
			builder.hostnameVerifier(new HostnameVerifier() {
				@Override
				public boolean verify(String hostname, SSLSession session) {
					return true;
				}
			});
			builder.cookieJar(jar);
			builder.writeTimeout(600, TimeUnit.SECONDS);

			OkHttpClient okHttpClient = builder.build();
			return okHttpClient;
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	public HeartyRabbit(String site, ContentResolver resolver, Context ctx)
	{
		m_site = site;
		m_resolver = resolver;

		m_cookieJar = new PersistentCookieJar(new SetCookieCache(), new SharedPrefsCookiePersistor(ctx));
		m_client = getUnsafeOkHttpClient(m_cookieJar);
	}

	public boolean is_login() throws Exception
	{
		HttpUrl url = new HttpUrl.Builder().scheme("https").host(m_site).build();

		List<Cookie> cookies = m_cookieJar.loadForRequest(url);
		for (Cookie c : cookies)
		if (c.name().equals("id") && !c.value().isEmpty())
		{
			Log.e("login", "horray!");
			return true;
		}
		return false;
	}

	public boolean login(String username, String password) throws Exception
	{
		m_user = username;

		RequestBody body = new FormBody.Builder()
			.add("username", username).add("password", password)
			.build();
		Request request = new Request.Builder()
			.url("https://" + m_site + "/login")
			.post(body)
			.build();
		try (Response response = m_client.newCall(request).execute())
		{
			// check if the session ID cookie is added to the cookie jar
			return is_login();
		}
	}

	private void upload(InputStream src, int size, String filename, String type) throws Exception
	{
		ByteArrayOutputStream byte_array = new ByteArrayOutputStream();

		byte[] buffer = new byte[4096*20];
		while (true) {
			int bytes = src.read(buffer);
			if (bytes == -1)
				break;

			Log.i("upload", "sent " + Integer.toString(bytes));

			byte_array.write(buffer, 0, bytes);
		}
		byte[] buf = byte_array.toByteArray();

		RequestBody body = RequestBody.create(MediaType.parse(type), buf);
		Request request = new Request.Builder()
			.url("https://" + m_site + "/upload/" + m_user + "/" + filename)
			.put(body)
			.build();
		try (Response response = m_client.newCall(request).execute())
		{
			Log.e("upload", "file updated" + Integer.toString(response.code()));
		}
	}

	public void upload(ArrayList<Uri> images)
	{
		for (Uri image : images)
		{
			try
			{
				Log.w("upload", "uploading " + image.toString());

				String[] projection = {OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE};

				String type = m_resolver.getType(image);
				Cursor c = m_resolver.query(image, projection, null, null, null);

				InputStream is = m_resolver.openInputStream(image);

				while (c.moveToNext())
				{
					int size_column = c.getColumnIndex(OpenableColumns.SIZE);
					Log.d("upload", "size = " + Integer.toString(c.getInt(size_column)));

					int name_column = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
					Log.d("upload", "name = " + c.getString(name_column));

					upload(is, c.getInt(size_column), c.getString(name_column), type);
				}
			}
			catch (Exception e)
			{
				Log.e("upload", e.toString());
			}
		}
	}
}
