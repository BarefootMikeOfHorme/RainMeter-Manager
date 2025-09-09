import os
import requests
import hashlib


def download_file(url, filepath, auth=None):
    try:
        with requests.get(url, auth=auth, stream=True) as r:
            r.raise_for_status()
            with open(filepath, 'wb') as f:
                for chunk in r.iter_content(chunk_size=8192):
                    f.write(chunk)
    except requests.exceptions.RequestException as e:
        print(f"Failed to download {url}: {e}")
        return False
    return True


def verify_integrity(filepath, expected_hash):
    hash_func = hashlib.sha256()
    with open(filepath, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_func.update(chunk)
    return hash_func.hexdigest() == expected_hash


def main():
    urls = [
        # Example URL and authentication details
        # {'url': 'http://example.com/file.rmskin', 'auth': ('user', 'pass'), 'hash': 'expectedhashhere'}
    ]

    for item in urls:
        url = item.get('url')
        auth = item.get('auth')
        expected_hash = item.get('hash')

        filename = url.split('/')[-1]
        saved_filepath = os.path.join('/d/RainmeterManager/samples/', filename)

        if download_file(url, saved_filepath, auth):
            if verify_integrity(saved_filepath, expected_hash):
                print(f"{filename} downloaded and verified successfully.")
            else:
                print(f"Integrity check failed for {filename}.")


if __name__ == "__main__":
    main()

