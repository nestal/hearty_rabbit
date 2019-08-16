// Encapsulation of what the user wants to do
class Intent
{
	constructor()
	{
		this.action = "view";
		this.owner  = null;
		this.coll   = null;
		this.filename = null;
		this.query  = null;
	}

	static current()
	{
		return Intent.from_url(new URL(document.URL));
	}

	static from_url(url)
	{
		let path = url.pathname.split('/');

		let r = new Intent();
		r.action = path[1];
		r.owner  = path[2];
		r.coll   = path[3];
		r.filename = path[4];
		r.query  = url.searchParams;

		console.assert(r != null);
		return r;
	}

	static collection(owner, coll)
	{
		let r = new Intent();
		r.owner = owner;
		r.coll = coll;
		return r;
	}

	static make(action, owner, coll, filename, query)
	{
		let r = new Intent();
		r.action = action;
		r.owner = owner;
		r.coll = coll;
		r.filename = filename;
		r.query = query;
		return r;
	}

	view(intent)
	{
		return Intent.make("view", this.owner, this.coll, this.filename, this.query);
	}

	api(intent)
	{
		return Intent.make("api", this.owner, this.coll, this.filename, this.query);
	}

	url()
	{
		let owner = this.owner;
		if (owner == null)
			owner = "";

		let collection = this.coll;
		if (collection == null)
			collection = "";

		let query = this.query;
		if (query == null && dir.auth != null)
			query = new URLSearchParams();
		if (dir.auth != null)
			query.set("auth", dir.auth);

		let filename = this.filename;
		if (filename == null)
			filename = "";

		return "/" + this.action +
			(owner.length      > 0 ? "/" : "") + owner +
			(collection.length > 0 ? "/" : "") + collection +
			(filename.length   > 0 ? "/" : "") + filename;
	}
}