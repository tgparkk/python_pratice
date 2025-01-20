import requests
from bs4 import BeautifulSoup
import time

def crawl_okky_gathering_list(page_limit=1):
    """
    Okky 모임/스터디 게시판 목록에서
    - 제목
    - 작성자
    - 작성 시각(목록상에서 노출된다면)
    등을 추출해 리스트로 반환합니다.

    page_limit: 가져올 페이지 수 (기본: 1)
    """
    base_url = "https://okky.kr/community/gathering"
    results = []

    for page_num in range(1, page_limit + 1):
        page_url = f"{base_url}?page={page_num}"  # 실제 파라미터 구조 확인 필수
        response = requests.get(page_url)
        response.raise_for_status()

        soup = BeautifulSoup(response.text, "html.parser")

        # (1) 'py-3.5 sm:py-4' 클래스를 포함하는 li 태그를 선택
        #    -> 문자열 전체("py-3.5 sm:py-4")를 부분 일치로 잡음
        post_items = soup.select('li[class*="py-3.5 sm:py-4"]')

        for li in post_items:
            # (2) 제목(<a>) 추출
            #    예: <a class="line-clamp-1 ...">게시글 제목</a>
            title_tag = li.select_one('a.line-clamp-1')
            if not title_tag:
                # li 안에 제목 링크 없으면 건너뜀
                continue

            title = title_tag.get_text(strip=True)
            href = title_tag.get("href", "")
            if href.startswith("/"):
                post_url = "https://okky.kr" + href
            else:
                post_url = href  # 혹시 절대경로로 있으면 그대로

            # (3) 작성자 / 작성시각
            #    예: <div class="flex items-center space-x-1 text-sm text-gray-500">박중호 · 6일 전</div>
            info_div = li.select_one('div.flex.items-center.space-x-1.text-sm.text-gray-500')
            if info_div:
                info_text = info_div.get_text(strip=True)
                # 예: "박중호 · 6일 전" 형태라면, '·'를 기준으로 분리
                parts = info_text.split('·')
                if len(parts) == 2:
                    author = parts[0].strip()
                    date_text = parts[1].strip()
                else:
                    author = "정보없음"
                    date_text = info_text
            else:
                author = "정보없음"
                date_text = "정보없음"

            results.append({
                "title": title,
                "url": post_url,
                "author": author,
                "date_text": date_text
            })

        # 페이지 간 간격 두기 (서버 부하 방지)
        time.sleep(1)

    return results


if __name__ == "__main__":
    data_list = crawl_okky_gathering_list(page_limit=1)
    print(f"수집된 게시글: {len(data_list)}개")
    for idx, item in enumerate(data_list, start=1):
        print(f"[{idx}] 제목: {item['title']}")
        print(f"    작성자: {item['author']}, 작성시각: {item['date_text']}")
        print(f"    URL: {item['url']}")
        print("-" * 70)
