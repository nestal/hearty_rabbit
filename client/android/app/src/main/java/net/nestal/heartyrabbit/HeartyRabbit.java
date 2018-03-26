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

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import okhttp3.FormBody;
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
	private CookieManager m_cookies;
	private ContentResolver m_resolver;

	OkHttpClient m_client = getUnsafeOkHttpClient();

	private static OkHttpClient getUnsafeOkHttpClient() {
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

			OkHttpClient okHttpClient = builder.build();
			return okHttpClient;
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	public HeartyRabbit(String site, ContentResolver resolver)
	{
		m_site = site;
		m_resolver = resolver;
	}

	public boolean login(String username, String password) throws Exception
	{
		RequestBody body = new FormBody.Builder()
			.add("username", "nestal").add("password", "hello")
			.build();
		Request request = new Request.Builder()
			.url("https://192.168.1.137:4433/login")
			.post(body)
			.build();
		try (Response response = m_client.newCall(request).execute())
		{
			Log.e("login", "cookie:" + response.header("Set-Cookie"));
			Log.e("login", "body:" + response.body().string());
		}

		return false;
	}

	private void upload(InputStream src, int size, String filename, String type) throws Exception
	{
		URL home_url = new URL("https://" + m_site + "/view/nestal/");
		HttpsURLConnection home_conn = (HttpsURLConnection)home_url.openConnection();
		home_conn.setDoInput(true);
		BufferedReader in = new BufferedReader(
			new InputStreamReader(home_conn.getInputStream()));
		String inputLine;
		StringBuffer response = new StringBuffer();

		while ((inputLine = in.readLine()) != null)
		{
			response.append(inputLine);
		}
		Log.e("gethome", response.toString());

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

		Log.i("upload", "read " + Integer.toString(buf.length));

		URL upload_url = new URL("https://" + m_site + "/upload/" + filename);
		HttpsURLConnection conn = (HttpsURLConnection)upload_url.openConnection();

		conn.setDoOutput(true);
		conn.setDoInput(true);

		conn.setRequestMethod("PUT");
		conn.setRequestProperty("Content-Type", type);
		conn.setRequestProperty("Content-Length", Integer.toString(size));
		conn.setInstanceFollowRedirects(false);

		conn.connect();
		OutputStream os = conn.getOutputStream();

		try( DataOutputStream wr = new DataOutputStream( conn.getOutputStream()))
		{
			wr.write(buf);
		}
		Log.i("upload", "response code: " + conn.getResponseCode());
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
