#!/usr/bin/env python3
"""Fix broken relative links in Discussion #1.

The discussion was created with relative links (../README.md) that resolve
to a 404 URL from the GitHub Discussions page. This script updates the
discussion body to use correct absolute links.
"""

import json
import os
import sys
import urllib.request

OWNER = "Claugo"
REPO = "segmented-sieve-wheel-m60-7"
DISCUSSION_NUMBER = 1

CORRECT_BODY = """\
👋 Welcome to the GC-60 Segmented Sieve project!

This is a space to discuss improvements, research directions, and optimizations.

**Quick Links:**

* 📖 [README](https://github.com/Claugo/segmented-sieve-wheel-m60-7/blob/main/README.md) - Overview and architecture
* 🤝 [CONTRIBUTING](https://github.com/Claugo/segmented-sieve-wheel-m60-7/blob/main/CONTRIBUTING.md) - How to contribute
* 🔬 [RESEARCH](https://github.com/Claugo/segmented-sieve-wheel-m60-7/blob/main/RESEARCH.md) - Open questions & ideas

**What We're Looking For:**

1. **Performance Optimization** - SIMD, multi-threading, cache tuning
2. **Algorithm Extensions** - Additional pre-filters, dual-level sieving
3. **Cross-Platform Support** - Testing on different CPUs/OSes
4. **Theoretical Analysis** - Correctness proofs, complexity bounds
5. **Academic Interest** - Paper drafts, mathematical formalization

**Getting Started:**

* Pick an item from RESEARCH.md that interests you
* Reply to this discussion with your ideas
* Open an Issue to propose changes
* Submit a PR when ready

Looking forward to collaborating! 🚀\
"""


BROKEN_LINK_MARKERS = ("../README.md", "../CONTRIBUTING.md", "../RESEARCH.md")


def graphql(query, variables=None):
    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if not token:
        print("Error: GH_TOKEN or GITHUB_TOKEN environment variable not set.", file=sys.stderr)
        sys.exit(1)

    payload = json.dumps({"query": query, "variables": variables or {}}).encode()
    req = urllib.request.Request(
        "https://api.github.com/graphql",
        data=payload,
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
            "Accept": "application/vnd.github+json",
        },
    )
    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read())
    except urllib.error.HTTPError as e:
        print(f"HTTP error {e.code}: {e.reason}", file=sys.stderr)
        sys.exit(1)
    except urllib.error.URLError as e:
        print(f"Network error: {e.reason}", file=sys.stderr)
        sys.exit(1)
    if "errors" in data:
        print(f"GraphQL errors: {json.dumps(data['errors'], indent=2)}", file=sys.stderr)
        sys.exit(1)
    return data


# Get current discussion body and node ID
get_query = """
query($owner: String!, $name: String!, $number: Int!) {
  repository(owner: $owner, name: $name) {
    discussion(number: $number) {
      id
      body
    }
  }
}
"""

print(f"Fetching Discussion #{DISCUSSION_NUMBER} from {OWNER}/{REPO}...")
data = graphql(get_query, {"owner": OWNER, "name": REPO, "number": DISCUSSION_NUMBER})

discussion = data["data"]["repository"]["discussion"]
discussion_id = discussion["id"]
current_body = discussion["body"]

print(f"Discussion ID: {discussion_id}")

if not any(marker in current_body for marker in BROKEN_LINK_MARKERS):
    print("Links already fixed — nothing to do.")
    sys.exit(0)

print("Found broken relative links. Updating discussion body...")

update_query = """
mutation($id: ID!, $body: String!) {
  updateDiscussion(input: { discussionId: $id, body: $body }) {
    discussion {
      id
      url
    }
  }
}
"""

result = graphql(update_query, {"id": discussion_id, "body": CORRECT_BODY})
updated = result["data"]["updateDiscussion"]["discussion"]
print(f"Discussion updated successfully: {updated['url']}")
