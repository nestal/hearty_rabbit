package net.nestal.heartyrabbitupload;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.TargetApi;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.app.LoaderManager.LoaderCallbacks;

import android.content.CursorLoader;
import android.content.Loader;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;

import android.os.Build;
import android.os.Bundle;
import android.provider.ContactsContract;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.DataOutputStream;
import java.net.CookieManager;
import java.net.URL;
import java.net.URLConnection;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import javax.net.ssl.HttpsURLConnection;

/**
 * A login screen that offers login via email/password.
 */
public class LoginActivity extends AppCompatActivity implements LoaderCallbacks<Cursor>
{
	/**
	 * A dummy authentication store containing known user names and passwords.
	 * TODO: remove after connecting to a real authentication system.
	 */
	private static final String[] DUMMY_CREDENTIALS = new String[]{
		"foo@example.com:hello", "bar@example.com:world"
	};
	/**
	 * Keep track of the login task to ensure we can cancel it if requested.
	 */
	private UserLoginTask m_auth_task = null;

	// UI references.
	private AutoCompleteTextView m_email;
	private EditText m_password;
	private View m_progress;
	private View m_login_form;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_login);
		// Set up the login form.
		m_email = (AutoCompleteTextView) findViewById(R.id.username);

		m_password = (EditText) findViewById(R.id.password);
		m_password.setOnEditorActionListener(new TextView.OnEditorActionListener()
		{
			@Override
			public boolean onEditorAction(TextView textView, int id, KeyEvent keyEvent)
			{
				if (id == EditorInfo.IME_ACTION_DONE || id == EditorInfo.IME_NULL)
				{
					attemptLogin();
					return true;
				}
				return false;
			}
		});

		Button mEmailSignInButton = (Button) findViewById(R.id.email_sign_in_button);
		mEmailSignInButton.setOnClickListener(new OnClickListener()
		{
			@Override
			public void onClick(View view)
			{
				attemptLogin();
			}
		});

		m_login_form = findViewById(R.id.login_form);
		m_progress = findViewById(R.id.login_progress);
	}

	/**
	 * Callback received when a permissions request has been completed.
	 */
	@Override
	public void onRequestPermissionsResult(
		int requestCode, @NonNull String[] permissions,
		@NonNull int[] grantResults
	)
	{
	}

	/**
	 * Attempts to sign in or register the account specified by the login form.
	 * If there are form errors (invalid email, missing fields, etc.), the
	 * errors are presented and no actual login attempt is made.
	 */
	private void attemptLogin()
	{
		if (m_auth_task != null)
		{
			return;
		}

		// Reset errors.
		m_email.setError(null);
		m_password.setError(null);

		// Store values at the time of the login attempt.
		String email = m_email.getText().toString();
		String password = m_password.getText().toString();

		boolean cancel = false;
		View focusView = null;

		// Check for a valid password, if the user entered one.
		if (!TextUtils.isEmpty(password) && !isPasswordValid(password))
		{
			m_password.setError(getString(R.string.error_invalid_password));
			focusView = m_password;
			cancel = true;
		}

		// Check for a valid email address.
		if (TextUtils.isEmpty(email))
		{
			m_email.setError(getString(R.string.error_field_required));
			focusView = m_email;
			cancel = true;
		} else if (!isEmailValid(email))
		{
			m_email.setError(getString(R.string.error_invalid_email));
			focusView = m_email;
			cancel = true;
		}

		if (cancel)
		{
			// There was an error; don't attempt login and focus the first
			// form field with an error.
			focusView.requestFocus();
		} else
		{
			// Show a progress spinner, and kick off a background task to
			// perform the user login attempt.
			showProgress(true);
			m_auth_task = new UserLoginTask(email, password);
			m_auth_task.execute((Void) null);
		}
	}

	private boolean isEmailValid(String email)
	{
		//TODO: Replace this with your own logic
		return true;
	}

	private boolean isPasswordValid(String password)
	{
		//TODO: Replace this with your own logic
		return password.length() > 4;
	}

	/**
	 * Shows the progress UI and hides the login form.
	 */
	@TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
	private void showProgress(final boolean show)
	{
		// On Honeycomb MR2 we have the ViewPropertyAnimator APIs, which allow
		// for very easy animations. If available, use these APIs to fade-in
		// the progress spinner.
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2)
		{
			int shortAnimTime = getResources().getInteger(android.R.integer.config_shortAnimTime);

			m_login_form.setVisibility(show ? View.GONE : View.VISIBLE);
			m_login_form.animate().setDuration(shortAnimTime).alpha(
				show ? 0 : 1).setListener(new AnimatorListenerAdapter()
			{
				@Override
				public void onAnimationEnd(Animator animation)
				{
					m_login_form.setVisibility(show ? View.GONE : View.VISIBLE);
				}
			});

			m_progress.setVisibility(show ? View.VISIBLE : View.GONE);
			m_progress.animate().setDuration(shortAnimTime).alpha(
				show ? 1 : 0).setListener(new AnimatorListenerAdapter()
			{
				@Override
				public void onAnimationEnd(Animator animation)
				{
					m_progress.setVisibility(show ? View.VISIBLE : View.GONE);
				}
			});
		} else
		{
			// The ViewPropertyAnimator APIs are not available, so simply show
			// and hide the relevant UI components.
			m_progress.setVisibility(show ? View.VISIBLE : View.GONE);
			m_login_form.setVisibility(show ? View.GONE : View.VISIBLE);
		}
	}

	@Override
	public Loader<Cursor> onCreateLoader(int i, Bundle bundle)
	{
		return new CursorLoader(this,
			// Retrieve data rows for the device user's 'profile' contact.
			Uri.withAppendedPath(
				ContactsContract.Profile.CONTENT_URI,
				ContactsContract.Contacts.Data.CONTENT_DIRECTORY
			), ProfileQuery.PROJECTION,

			// Select only email addresses.
			ContactsContract.Contacts.Data.MIMETYPE +
				" = ?", new String[]{
			ContactsContract.CommonDataKinds.Email
				.CONTENT_ITEM_TYPE
		},

			// Show primary email addresses first. Note that there won't be
			// a primary email address if the user hasn't specified one.
			ContactsContract.Contacts.Data.IS_PRIMARY + " DESC"
		);
	}

	@Override
	public void onLoadFinished(Loader<Cursor> cursorLoader, Cursor cursor)
	{
		List<String> emails = new ArrayList<>();
		cursor.moveToFirst();
		while (!cursor.isAfterLast())
		{
			emails.add(cursor.getString(ProfileQuery.ADDRESS));
			cursor.moveToNext();
		}

		addEmailsToAutoComplete(emails);
	}

	@Override
	public void onLoaderReset(Loader<Cursor> cursorLoader)
	{

	}

	private void addEmailsToAutoComplete(List<String> emailAddressCollection)
	{
		//Create adapter to tell the AutoCompleteTextView what to show in its dropdown list.
		ArrayAdapter<String> adapter =
			new ArrayAdapter<>(LoginActivity.this,
				android.R.layout.simple_dropdown_item_1line, emailAddressCollection
			);

		m_email.setAdapter(adapter);
	}


	private interface ProfileQuery
	{
		String[] PROJECTION = {
			ContactsContract.CommonDataKinds.Email.ADDRESS,
			ContactsContract.CommonDataKinds.Email.IS_PRIMARY,
		};

		int ADDRESS = 0;
		int IS_PRIMARY = 1;
	}

	/**
	 * Represents an asynchronous login/registration task used to authenticate
	 * the user.
	 */
	public class UserLoginTask extends AsyncTask<Void, Void, Boolean>
	{

		private final String mEmail;
		private final String mPassword;

		UserLoginTask(String email, String password)
		{
			mEmail = email;
			mPassword = password;
		}

		@Override
		protected Boolean doInBackground(Void... params)
		{
			// TODO: attempt authentication against a network service.

			try
			{
				CookieManager cookie = new CookieManager();


				URL login_url = new URL("https://www.nestal.net/login");
				HttpsURLConnection conn = (HttpsURLConnection)login_url.openConnection();
				conn.setRequestMethod("POST");
				conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");

				byte[] data = new String("username=" + mEmail + "&password=" + mPassword).getBytes(StandardCharsets.UTF_8);
				conn.setRequestProperty("Content-Length", Integer.toString(data.length));

				conn.getDoOutput();
				try( DataOutputStream wr = new DataOutputStream( conn.getOutputStream()))
				{
					wr.write(data);
				}

				conn.connect();

			} catch (Exception e)
			{
				Log.e("Login", "Login exception: " + e.toString());
				return false;
			}

			for (String credential : DUMMY_CREDENTIALS)
			{
				String[] pieces = credential.split(":");
				if (pieces[0].equals(mEmail))
				{
					// Account exists, return true if the password matches.
					return pieces[1].equals(mPassword);
				}
			}

			// TODO: register the new account here.
			return true;
		}

		@Override
		protected void onPostExecute(final Boolean success)
		{
			m_auth_task = null;
			showProgress(false);

			if (success)
			{
				finish();
			} else
			{
				m_password.setError(getString(R.string.error_incorrect_password));
				m_password.requestFocus();
			}
		}

		@Override
		protected void onCancelled()
		{
			m_auth_task = null;
			showProgress(false);
		}
	}
}

